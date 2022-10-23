#pragma once
#include "core_types.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	enum class SprGroup : u8
	{
		Timeline,
		Game,
		Count
	};

	enum class SprID : u8
	{
		Timeline_Note_Don,
		Timeline_Note_DonBig,
		Timeline_Note_Ka,
		Timeline_Note_KaBig,
		Timeline_Note_Drumroll,
		Timeline_Note_DrumrollBig,
		Timeline_Note_DrumrollLong,
		Timeline_Note_DrumrollLongBig,
		Timeline_Note_Balloon,
		Timeline_Note_BalloonSpecial,
		Timeline_Note_BalloonLong,
		Timeline_Note_BalloonLongSpecial,

		Game_Note_Don,
		Game_Note_DonBig,
		Game_Note_Ka,
		Game_Note_KaBig,
		Game_Note_Drumroll,
		Game_Note_DrumrollBig,
		Game_Note_DrumrollLong,
		Game_Note_DrumrollLongBig,
		Game_Note_Balloon,
		Game_Note_BalloonSpecial,
		Game_NoteTxt_Do,
		Game_NoteTxt_Don,
		Game_NoteTxt_DonBig,
		Game_NoteTxt_Ka,
		Game_NoteTxt_Katsu,
		Game_NoteTxt_KatsuBig,
		Game_NoteTxt_Drumroll,
		Game_NoteTxt_DrumrollBig,
		Game_NoteTxt_Balloon,
		Game_NoteTxt_BalloonSpecial,
		// TODO: Split into individual sprites to correctly handle padding (?)
		Game_Font_Numerical,

		Count
	};

	struct SprTypeDesc { SprID Spr; SprGroup Group; cstr FilePath; f32 BaseScale; };
	constexpr SprTypeDesc SprDescTable[] =
	{
		{ SprID::Timeline_Note_Don,					SprGroup::Timeline, u8"assets/graphics/timeline_note_don.svg" },
		{ SprID::Timeline_Note_DonBig,				SprGroup::Timeline, u8"assets/graphics/timeline_note_don_big.svg" },
		{ SprID::Timeline_Note_Ka,					SprGroup::Timeline, u8"assets/graphics/timeline_note_ka.svg" },
		{ SprID::Timeline_Note_KaBig,				SprGroup::Timeline, u8"assets/graphics/timeline_note_ka_big.svg" },
		{ SprID::Timeline_Note_Drumroll,			SprGroup::Timeline, u8"assets/graphics/timeline_note_renda.svg" },
		{ SprID::Timeline_Note_DrumrollBig,			SprGroup::Timeline, u8"assets/graphics/timeline_note_renda_big.svg" },
		{ SprID::Timeline_Note_DrumrollLong,		SprGroup::Timeline, u8"assets/graphics/timeline_note_renda_long.svg" },
		{ SprID::Timeline_Note_DrumrollLongBig,		SprGroup::Timeline, u8"assets/graphics/timeline_note_renda_long_big.svg" },
		{ SprID::Timeline_Note_Balloon,				SprGroup::Timeline, u8"assets/graphics/timeline_note_fuusen.svg" },
		{ SprID::Timeline_Note_BalloonSpecial,		SprGroup::Timeline, u8"assets/graphics/timeline_note_fuusen_big.svg" },
		{ SprID::Timeline_Note_BalloonLong,			SprGroup::Timeline, u8"assets/graphics/timeline_note_fuusen_long.svg" },
		{ SprID::Timeline_Note_BalloonLongSpecial,	SprGroup::Timeline, u8"assets/graphics/timeline_note_fuusen_long_big.svg" },

		{ SprID::Game_Note_Don,						SprGroup::Game, u8"assets/graphics/game_note_don.svg" },
		{ SprID::Game_Note_DonBig,					SprGroup::Game, u8"assets/graphics/game_note_don_big.svg" },
		{ SprID::Game_Note_Ka,						SprGroup::Game, u8"assets/graphics/game_note_ka.svg" },
		{ SprID::Game_Note_KaBig,					SprGroup::Game, u8"assets/graphics/game_note_ka_big.svg" },
		{ SprID::Game_Note_Drumroll,				SprGroup::Game, u8"assets/graphics/game_note_renda.svg" },
		{ SprID::Game_Note_DrumrollBig,				SprGroup::Game, u8"assets/graphics/game_note_renda_big.svg" },
		{ SprID::Game_Note_DrumrollLong,			SprGroup::Game, u8"assets/graphics/game_note_renda_long.svg" },
		{ SprID::Game_Note_DrumrollLongBig,			SprGroup::Game, u8"assets/graphics/game_note_renda_long_big.svg" },
		{ SprID::Game_Note_Balloon,					SprGroup::Game, u8"assets/graphics/game_note_fuusen.svg" },
		{ SprID::Game_Note_BalloonSpecial,			SprGroup::Game, u8"assets/graphics/game_note_fuusen_big.svg" },
		{ SprID::Game_NoteTxt_Do,					SprGroup::Game, u8"assets/graphics/game_note_txt_do.svg" },
		{ SprID::Game_NoteTxt_Don,					SprGroup::Game, u8"assets/graphics/game_note_txt_don.svg" },
		{ SprID::Game_NoteTxt_DonBig,				SprGroup::Game, u8"assets/graphics/game_note_txt_don_big.svg" },
		{ SprID::Game_NoteTxt_Ka,					SprGroup::Game, u8"assets/graphics/game_note_txt_ka.svg" },
		{ SprID::Game_NoteTxt_Katsu,				SprGroup::Game, u8"assets/graphics/game_note_txt_katsu.svg" },
		{ SprID::Game_NoteTxt_KatsuBig,				SprGroup::Game, u8"assets/graphics/game_note_txt_katsu_big.svg" },
		{ SprID::Game_NoteTxt_Drumroll,				SprGroup::Game, u8"assets/graphics/game_note_txt_renda.svg" },
		{ SprID::Game_NoteTxt_DrumrollBig,			SprGroup::Game, u8"assets/graphics/game_note_txt_renda_big.svg" },
		{ SprID::Game_NoteTxt_Balloon,				SprGroup::Game, u8"assets/graphics/game_note_txt_fuusen.svg" },
		{ SprID::Game_NoteTxt_BalloonSpecial,		SprGroup::Game, u8"assets/graphics/game_note_txt_fuusen_big.svg" },
		// TODO: ...
		{ SprID::Game_Font_Numerical,				SprGroup::Game, u8"assets/graphics/game_font_numerical.svg", 0.5f },
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
