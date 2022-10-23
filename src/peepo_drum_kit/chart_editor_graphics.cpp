#include "chart_editor_graphics.h"
#include "core_io.h"
#include <thorvg/thorvg.h>
#include <thread>
#include <future>

// TODO: Use for packing texture atlases (?)
// #include "imgui/3rdparty/imstb_rectpack.h"

namespace PeepoDrumKit
{
	static constexpr b8 CompileTimeValidateSprDescTable()
	{
		for (size_t i = 0; i < ArrayCount(SprDescTable); i++)
			if (SprDescTable[i].Spr != static_cast<SprID>(i))
				return false;
		return (ArrayCount(SprDescTable) == EnumCount<SprID>);
	}
	static_assert(CompileTimeValidateSprDescTable(), "Missing or bad entry in sprite table");

	static constexpr i32 PerSideRasterizedTexPadding = 2;
	static constexpr i32 CombinedRasterizedTexPadding = (PerSideRasterizedTexPadding * 2);

	struct RasterizedBitmap { std::unique_ptr<u32[]> BGRA; ivec2 Resolution; };
	struct SvgRasterizer
	{
		std::unique_ptr<tvg::SwCanvas> Canvas = nullptr;
		tvg::Picture* PictureView = nullptr;
		vec2 PictureSize = {};
		f32 BaseScale;

		void ParseSVG(std::string_view svgFileContent, f32 baseScale)
		{
			auto picture = tvg::Picture::gen();
			picture->load(svgFileContent.data(), static_cast<u32>(svgFileContent.size()), "svg", false);
			picture->size(&PictureSize.x, &PictureSize.y);
			PictureSize *= baseScale;
			BaseScale = baseScale;
			PictureView = picture.get();

			assert(Canvas == nullptr);
			Canvas = tvg::SwCanvas::gen();
			Canvas->push(std::move(picture));
		}

		// TODO: Rasterize directly into final atlas using stride
		RasterizedBitmap Rasterize(f32 scale)
		{
			const vec2 scaledPictureSize = (PictureSize * scale);
			const ivec2 resolutionWithoutPadding = { static_cast<i32>(Ceil(scaledPictureSize.x)), static_cast<i32>(Ceil(scaledPictureSize.y)) };
			const ivec2 resolution = resolutionWithoutPadding + ivec2(CombinedRasterizedTexPadding);
			RasterizedBitmap out { std::make_unique<u32[]>(resolution.x * resolution.y), resolution };
			if (resolutionWithoutPadding.x <= 0 || resolutionWithoutPadding.y <= 0)
				return out;

			const vec2 position = vec2(PerSideRasterizedTexPadding, PerSideRasterizedTexPadding);
			PictureView->scale(scale * BaseScale);
			PictureView->translate(position.x, position.y);

			Canvas->target(out.BGRA.get(), resolution.x, resolution.x, resolution.y, tvg::SwCanvas::ARGB8888/*_STRAIGHT*/);
			Canvas->update(PictureView);
			Canvas->draw();
			Canvas->sync();

			if constexpr (PerSideRasterizedTexPadding > 0)
			{
				auto pixelAt = [&](i32 x, i32 y) -> u32& { return out.BGRA[(y * resolution.x) + x]; };
				auto pixelAtWithoutPadding = [&](i32 x, i32 y) -> u32& { return pixelAt(x + PerSideRasterizedTexPadding, y + PerSideRasterizedTexPadding); };

				for (i32 x = 0; x < PerSideRasterizedTexPadding; x++)
					for (i32 y = 0; y < PerSideRasterizedTexPadding; y++) // NOTE: Top-left / bottom-left / top-right / bottom-right corner
					{
						const ivec2 tl = ivec2(x, y);
						const ivec2 br = ivec2(x, y) + (resolutionWithoutPadding + ivec2(PerSideRasterizedTexPadding));
						pixelAt(tl.x, tl.y) = pixelAtWithoutPadding(0, 0);
						pixelAt(tl.x, br.y) = pixelAtWithoutPadding(0, resolutionWithoutPadding.y - 1);
						pixelAt(br.x, tl.y) = pixelAtWithoutPadding(resolutionWithoutPadding.x - 1, 0);
						pixelAt(br.x, br.y) = pixelAtWithoutPadding(resolutionWithoutPadding.x - 1, resolutionWithoutPadding.y - 1);
					}

				for (i32 y = 0; y < PerSideRasterizedTexPadding; y++)
					for (i32 x = 0; x < resolutionWithoutPadding.x; x++) // NOTE: Top / bottom row
					{
						pixelAt(PerSideRasterizedTexPadding + x, y) = pixelAtWithoutPadding(x, 0);
						pixelAt(PerSideRasterizedTexPadding + x, resolutionWithoutPadding.y + y + PerSideRasterizedTexPadding) = pixelAtWithoutPadding(x, resolutionWithoutPadding.y - 1);
					}
				for (i32 y = 0; y < resolutionWithoutPadding.y; y++)
					for (i32 x = 0; x < PerSideRasterizedTexPadding; x++) // NOTE: Left / right row
					{
						pixelAt(x, PerSideRasterizedTexPadding + y) = pixelAtWithoutPadding(0, y);
						pixelAt(resolutionWithoutPadding.x + x + PerSideRasterizedTexPadding, PerSideRasterizedTexPadding + y) = pixelAtWithoutPadding(resolutionWithoutPadding.x - 1, y);
					}
			}

			return out;
		}
	};

	struct ChartGraphicsResources::OpaqueData
	{
		f32 PerGroupRasterScale[EnumCount<SprGroup>];

		b8 FinishedLoading;
		std::future<void> LoadFuture;

		// TODO: Combine multiple spirites into texture atlases (?)
		SvgRasterizer PerSprSvg[EnumCount<SprID>];
		CustomDraw::GPUTexture PerSprTexture[EnumCount<SprID>];

		// TODO: Global alpha to handle async load fade-ins (?)
		// f32 PerGroupGlobalAlpha[EnumCount<SprGroup>];
	};

	ChartGraphicsResources::ChartGraphicsResources()
	{
		const u32 threadCount = static_cast<u32>(ClampBot(static_cast<i32>(std::thread::hardware_concurrency()) - 1, 0));
		tvg::Initializer::init(tvg::CanvasEngine::Sw, threadCount);

		Data = std::make_unique<OpaqueData>();
	}

	ChartGraphicsResources::~ChartGraphicsResources()
	{
		for (auto& it : Data->PerSprTexture) { it.Unload(); }
	}

	void ChartGraphicsResources::StartAsyncLoading()
	{
		assert(!Data->LoadFuture.valid());
		Data->FinishedLoading = false;
		Data->LoadFuture = std::async(std::launch::async, [this]()
		{
#if PEEPO_DEBUG // DEBUG: ...
			auto sw = CPUStopwatch::StartNew();
			defer { auto elapsed = sw.Stop(); printf("Took %g ms to load all SVGs\n", elapsed.ToMS()); };
#endif

			for (const SprTypeDesc& it : SprDescTable)
			{
				auto fileContent = File::ReadAllBytes(it.FilePath);
#if PEEPO_DEBUG // DEBUG: ...
				if (fileContent.Content == nullptr)
					printf("Failed to read sprite file '%s'\n", it.FilePath);
#endif

				Data->PerSprSvg[EnumToIndex(it.Spr)].ParseSVG(fileContent.AsString(), (it.BaseScale != 0.0f) ? it.BaseScale : 1.0f);
			}
		});
	}

	void ChartGraphicsResources::UpdateAsyncLoading()
	{
		if (Data->LoadFuture.valid() && Data->LoadFuture._Is_ready())
		{
			Data->LoadFuture.get();
			Data->FinishedLoading = true;
		}
	}

	b8 ChartGraphicsResources::IsAsyncLoading() const
	{
		return Data->LoadFuture.valid();
	}

	void ChartGraphicsResources::Rasterize(SprGroup group, f32 scale)
	{
		assert(Data->FinishedLoading && group < SprGroup::Count);

		f32& currentRasterScale = Data->PerGroupRasterScale[EnumToIndex(group)];
		if (ApproxmiatelySame(currentRasterScale, scale))
			return;
		currentRasterScale = scale;

		for (i32 sprIndex = 0; sprIndex < EnumCountI32<SprID>; sprIndex++)
		{
			if (GetSprGroup(static_cast<SprID>(sprIndex)) != group)
				continue;

			auto bitmap = Data->PerSprSvg[sprIndex].Rasterize(currentRasterScale);
			Data->PerSprTexture[sprIndex].Unload();
			if (bitmap.Resolution.x > 0 && bitmap.Resolution.y > 0)
				Data->PerSprTexture[sprIndex].Load(CustomDraw::GPUTextureDesc { CustomDraw::GPUPixelFormat::BGRA, CustomDraw::GPUAccessType::Static, bitmap.Resolution, bitmap.BGRA.get() });
		}
	}

	SprInfo ChartGraphicsResources::GetInfo(SprID spr) const
	{
		if (!Data->FinishedLoading || spr >= SprID::Count)
			return {};

		SprInfo info;
		info.SourceSize = Data->PerSprSvg[EnumToIndex(spr)].PictureSize;
		info.RasterScale = Data->PerGroupRasterScale[EnumToIndex(GetSprGroup(spr))];
		return info;
	}

	b8 ChartGraphicsResources::GetImageQuad(ImImageQuad& out, SprID spr, SprTransform transform, u32 colorTint, const SprUV* uv)
	{
		if (!Data->FinishedLoading || spr >= SprID::Count)
			return false;

		const f32 rasterScale = Data->PerGroupRasterScale[EnumToIndex(GetSprGroup(spr))];;
		transform.Scale /= rasterScale;
		const vec2 scaledPictureSize = Data->PerSprSvg[EnumToIndex(spr)].PictureSize * rasterScale;

		const auto& tex = Data->PerSprTexture[EnumToIndex(spr)];
		const vec2 texSizePadded = tex.GetSizeF32();
		const vec2 size = (transform.Scale * scaledPictureSize);
		const vec2 pivot = (-transform.Pivot * transform.Scale * scaledPictureSize);

		vec2 tl, tr, bl, br;
		if (transform.Rotation.Radians == 0.0f)
		{
			const vec2 pos = (transform.Position + pivot);
			tl.x = pos.x;
			tl.y = pos.y;
			tr.x = pos.x + size.x;
			tr.y = pos.y;
			bl.x = pos.x;
			bl.y = pos.y + size.y;
			br.x = pos.x + size.x;
			br.y = pos.y + size.y;
		}
		else
		{
			const vec2 pos = transform.Position;
			const f32 sin = Sin(transform.Rotation);
			const f32 cos = Cos(transform.Rotation);
			tl.x = pos.x + pivot.x * cos - pivot.y * sin;
			tl.y = pos.y + pivot.x * sin + pivot.y * cos;
			tr.x = pos.x + (pivot.x + size.x) * cos - pivot.y * sin;
			tr.y = pos.y + (pivot.x + size.x) * sin + pivot.y * cos;
			bl.x = pos.x + pivot.x * cos - (pivot.y + size.y) * sin;
			bl.y = pos.y + pivot.x * sin + (pivot.y + size.y) * cos;
			br.x = pos.x + (pivot.x + size.x) * cos - (pivot.y + size.y) * sin;
			br.y = pos.y + (pivot.x + size.x) * sin + (pivot.y + size.y) * cos;
		}

		vec2 quadUV[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		if (uv != nullptr) { quadUV[0] = uv->TL; quadUV[1] = uv->TR; quadUV[2] = uv->BR; quadUV[3] = uv->BL; }

		for (vec2& it : quadUV)
		{
			it *= scaledPictureSize;
			it += vec2(PerSideRasterizedTexPadding, PerSideRasterizedTexPadding);
			it /= texSizePadded;
		}

		out.TexID = tex.GetTexID();
		out.Pos[0] = tl; out.Pos[1] = tr;
		out.Pos[2] = br; out.Pos[3] = bl;
		out.UV[0] = quadUV[0]; out.UV[1] = quadUV[1];
		out.UV[2] = quadUV[2]; out.UV[3] = quadUV[3];
		out.Color = colorTint;
		return true;
	}

	SprStretchtOut StretchMultiPartSpr(ChartGraphicsResources& gfx, SprID spr, SprTransform transform, u32 color, SprStretchtParam param, size_t splitCount)
	{
		// TODO: "i32 Axis" param for x/y (?)
		SprStretchtOut out {};

		// TODO: ...
		//param.Splits[...];

		if (splitCount == 2)
		{
			static constexpr f32 perSplitPercentage = (1.0f / 2.0f);;
			transform.Scale.x *= (param.Scales[0] + param.Scales[1]) / 2.0f;
			ImImageQuad baseQuad {}; gfx.GetImageQuad(baseQuad, spr, transform, color, nullptr);

			const f32 splitT = param.Scales[0] / (param.Scales[1] + param.Scales[0]);
			const vec2 topM = Lerp<vec2>(baseQuad.TL(), baseQuad.TR(), splitT);
			const vec2 botM = Lerp<vec2>(baseQuad.BL(), baseQuad.BR(), splitT);
			const vec2 uvTopM = Lerp<vec2>(baseQuad.UV_TL(), baseQuad.UV_TR(), perSplitPercentage);
			const vec2 uvBotM = Lerp<vec2>(baseQuad.UV_BL(), baseQuad.UV_BR(), perSplitPercentage);

			out.BaseTransform = transform;
			out.Quads[0] = baseQuad;
			out.Quads[0].TR(topM); out.Quads[0].UV_TR(uvTopM);
			out.Quads[0].BR(botM); out.Quads[0].UV_BR(uvBotM);

			out.Quads[1] = baseQuad;
			out.Quads[1].TL(topM); out.Quads[1].UV_TL(uvTopM);
			out.Quads[1].BL(botM); out.Quads[1].UV_BL(uvBotM);
		}
		else if (splitCount == 3)
		{
			static constexpr f32 perSplitPercentage = (1.0f / 3.0f);;
			transform.Scale.x *= (param.Scales[0] + param.Scales[1] + param.Scales[2]) / 3.0f;
			ImImageQuad baseQuad {}; gfx.GetImageQuad(baseQuad, spr, transform, color, nullptr);

			const f32 splitLT = param.Scales[0] / (param.Scales[2] + param.Scales[1] + param.Scales[0]);
			const f32 splitRT = 1.0f - (param.Scales[2] / (param.Scales[2] + param.Scales[1] + param.Scales[0]));
			const vec2 topLM = Lerp<vec2>(baseQuad.TL(), baseQuad.TR(), splitLT);
			const vec2 botLM = Lerp<vec2>(baseQuad.BL(), baseQuad.BR(), splitLT);
			const vec2 topRM = Lerp<vec2>(baseQuad.TL(), baseQuad.TR(), splitRT);
			const vec2 botRM = Lerp<vec2>(baseQuad.BL(), baseQuad.BR(), splitRT);
			const vec2 uvTopLM = Lerp<vec2>(baseQuad.UV_TL(), baseQuad.UV_TR(), perSplitPercentage);
			const vec2 uvBotLM = Lerp<vec2>(baseQuad.UV_BL(), baseQuad.UV_BR(), perSplitPercentage);
			const vec2 uvTopRM = Lerp<vec2>(baseQuad.UV_TL(), baseQuad.UV_TR(), 1.0f - perSplitPercentage);
			const vec2 uvBotRM = Lerp<vec2>(baseQuad.UV_BL(), baseQuad.UV_BR(), 1.0f - perSplitPercentage);

			out.BaseTransform = transform;
			out.Quads[0] = baseQuad;
			out.Quads[0].TR(topLM); out.Quads[0].UV_TR(uvTopLM);
			out.Quads[0].BR(botLM); out.Quads[0].UV_BR(uvBotLM);

			out.Quads[1] = baseQuad;
			out.Quads[1].TL(topLM); out.Quads[1].UV_TL(uvTopLM);
			out.Quads[1].BL(botLM); out.Quads[1].UV_BL(uvBotLM);
			out.Quads[1].TR(topRM); out.Quads[1].UV_TR(uvTopRM);
			out.Quads[1].BR(botRM); out.Quads[1].UV_BR(uvBotRM);

			out.Quads[2] = baseQuad;
			out.Quads[2].TL(topRM); out.Quads[2].UV_TL(uvTopRM);
			out.Quads[2].BL(botRM); out.Quads[2].UV_BL(uvBotRM);
		}
		else
		{
			assert(false);
		}

		return out;
	}
}
