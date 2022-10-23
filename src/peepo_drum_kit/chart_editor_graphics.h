#pragma once
#include "core_types.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	enum class SprGroup : u8
	{
		Count
	};

	enum class SprID : u8
	{
		Count
	};

	struct SprTypeDesc { SprID Spr; SprGroup Group; cstr FilePath; f32 BaseScale; };
	constexpr SprTypeDesc SprDescTable[] =
	{
	};

	constexpr SprGroup GetSprGroup(SprID spr) { return (spr < SprID::Count) ? SprDescTable[EnumToIndex(spr)].Group : SprGroup::Count; }

	struct SprInfo { vec2 SourceSize; f32 RasterScale; };

	struct SprTransform
	{
		vec2 Position;	// NOTE: In absolute screen space
		vec2 Pivot;		// NOTE: In normalized 0 to 1 range
		vec2 Scale;		// NOTE: Around pivot
		Angle Rotation; // NOTE: Around pivot

		static constexpr SprTransform FromTL(vec2 tl, vec2 scale = vec2(1.0f), Angle rot = {}) { return { tl, vec2(0.0f, 0.0f), scale, rot }; }
		static constexpr SprTransform FromTR(vec2 tr, vec2 scale = vec2(1.0f), Angle rot = {}) { return { tr, vec2(1.0f, 0.0f), scale, rot }; }
		static constexpr SprTransform FromBL(vec2 bl, vec2 scale = vec2(1.0f), Angle rot = {}) { return { bl, vec2(0.0f, 1.0f), scale, rot }; }
		static constexpr SprTransform FromBR(vec2 br, vec2 scale = vec2(1.0f), Angle rot = {}) { return { br, vec2(1.0f, 1.0f), scale, rot }; }
		static constexpr SprTransform FromCenter(vec2 center, vec2 scale = vec2(1.0f), Angle rot = {}) { return { center, vec2(0.5f, 0.5f), scale, rot }; }
		static constexpr SprTransform FromCenterL(vec2 center, vec2 scale = vec2(1.0f), Angle rot = {}) { return { center, vec2(0.0f, 0.5f), scale, rot }; }
		static constexpr SprTransform FromCenterR(vec2 center, vec2 scale = vec2(1.0f), Angle rot = {}) { return { center, vec2(1.0f, 0.5f), scale, rot }; }
		constexpr SprTransform operator+(vec2 offset) const { SprTransform copy = *this; copy.Position += offset; return copy; }
		constexpr SprTransform operator-(vec2 offset) const { SprTransform copy = *this; copy.Position -= offset; return copy; }
	};

	// NOTE: Normalized (0.0f <-> 1.0f) per quad corner
	struct SprUV
	{
		vec2 TL, TR, BL, BR;
		static constexpr SprUV FromRect(vec2 tl, vec2 br) { return SprUV { tl, vec2(br.x, tl.y), vec2(tl.x, br.y), br }; }
	};

	// NOTE: Stored in the the Dear ImGui native clockwise triangle winding order of { TL, TR, BR, BL }
	struct ImImageQuad
	{
		ImTextureID TexID; ImVec2 Pos[4]; ImVec2 UV[4]; ImU32 Color;
		inline ImVec2 TL() const { return Pos[0]; } inline void TL(ImVec2 v) { Pos[0] = v; }
		inline ImVec2 TR() const { return Pos[1]; } inline void TR(ImVec2 v) { Pos[1] = v; }
		inline ImVec2 BR() const { return Pos[2]; } inline void BR(ImVec2 v) { Pos[2] = v; }
		inline ImVec2 BL() const { return Pos[3]; } inline void BL(ImVec2 v) { Pos[3] = v; }
		inline ImVec2 UV_TL() const { return UV[0]; } inline void UV_TL(ImVec2 v) { UV[0] = v; }
		inline ImVec2 UV_TR() const { return UV[1]; } inline void UV_TR(ImVec2 v) { UV[1] = v; }
		inline ImVec2 UV_BR() const { return UV[2]; } inline void UV_BR(ImVec2 v) { UV[2] = v; }
		inline ImVec2 UV_BL() const { return UV[3]; } inline void UV_BL(ImVec2 v) { UV[3] = v; }
	};

	struct ChartGraphicsResources : NonCopyable
	{
		ChartGraphicsResources();
		~ChartGraphicsResources();

		void StartAsyncLoading();
		void UpdateAsyncLoading();
		b8 IsAsyncLoading() const;

		// TODO: Only actually rebulid textures after *completeing* a resize (?)
		void Rasterize(SprGroup group, f32 scale);

		SprInfo GetInfo(SprID spr) const;
		b8 GetImageQuad(ImImageQuad& out, SprID spr, SprTransform transform, u32 colorTint, const SprUV* uv);

		inline void DrawSprite(ImDrawList* drawList, const ImImageQuad& quad) { drawList->AddImageQuad(quad.TexID, quad.Pos[0], quad.Pos[1], quad.Pos[2], quad.Pos[3], quad.UV[0], quad.UV[1], quad.UV[2], quad.UV[3], quad.Color); }
		inline void DrawSprite(ImDrawList* drawList, SprID spr, SprTransform transform, u32 colorTint = 0xFFFFFFFF, const SprUV* uv = nullptr) { if (ImImageQuad quad; GetImageQuad(quad, spr, transform, colorTint, uv)) { DrawSprite(drawList, quad); } }

		struct OpaqueData;
		std::unique_ptr<OpaqueData> Data;
	};

	// NOTE: Per segment scale factor and per split normalized percentage
	struct SprStretchtParam { f32 Scales[3]; /*f32 Splits[3];*/ };
	struct SprStretchtOut { SprTransform BaseTransform; ImImageQuad Quads[3]; };
	// NOTE: Take a sprite of source transform, split it into N parts and stretch those parts by varying amounts without gaps as if they were one
	SprStretchtOut StretchMultiPartSpr(ChartGraphicsResources& gfx, SprID spr, SprTransform transform, u32 color, SprStretchtParam param, size_t splitCount);
}
