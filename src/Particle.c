#include "Particle.h"
#include "Block.h"
#include "World.h"
#include "ExtMath.h"
#include "Lighting.h"
#include "Entity.h"
#include "TexturePack.h"
#include "Graphics.h"
#include "Funcs.h"
#include "Game.h"
#include "Event.h"
#include "GameStructs.h"


/*########################################################################################################################*
*------------------------------------------------------Particle base------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Particles_TexId, Particles_VB;
#define PARTICLES_MAX 600
static RNGState rnd;
static bool particle_hitTerrain;

void Particle_DoRender(Vec2* size, Vec3* pos, TextureRec* rec, PackedCol col, VertexP3fT2fC4b* vertices) {
	struct Matrix* view;
	VertexP3fT2fC4b v;
	float sX, sY;
	Vec3 centre;
	float aX, aY, aZ, bX, bY, bZ;

	sX = size->X * 0.5f; sY = size->Y * 0.5f;
	centre = *pos; centre.Y += sY;
	view   = &Gfx.View;
	
	aX = view->Row0.X * sX; aY = view->Row1.X * sX; aZ = view->Row2.X * sX; /* right * size.X * 0.5f */
	bX = view->Row0.Y * sY; bY = view->Row1.Y * sY; bZ = view->Row2.Y * sY; /* up    * size.Y * 0.5f */
	v.Col = col;

	v.X = centre.X - aX - bX; v.Y = centre.Y - aY - bY; v.Z = centre.Z - aZ - bZ;
	v.U = rec->U1; v.V = rec->V2; vertices[0] = v;

	v.X = centre.X - aX + bX; v.Y = centre.Y - aY + bY; v.Z = centre.Z - aZ + bZ;
				   v.V = rec->V1; vertices[1] = v;

	v.X = centre.X + aX + bX; v.Y = centre.Y + aY + bY; v.Z = centre.Z + aZ + bZ;
	v.U = rec->U2;                vertices[2] = v;

	v.X = centre.X + aX - bX; v.Y = centre.Y + aY - bY; v.Z = centre.Z + aZ - bZ;
				   v.V = rec->V2; vertices[3] = v;
}

static void Particle_Reset(struct Particle* p, Vec3 pos, Vec3 velocity, float lifetime) {
	p->lastPos = pos; p->nextPos = pos;
	p->velocity = velocity;
	p->lifetime = lifetime;
}

static bool Particle_CanPass(BlockID block, bool throughLiquids) {
	uint8_t draw = Blocks.Draw[block];
	return draw == DRAW_GAS || draw == DRAW_SPRITE || (throughLiquids && Blocks.IsLiquid[block]);
}

static bool Particle_CollideHor(Vec3* nextPos, BlockID block) {
	Vec3 horPos = Vec3_Create3((float)Math_Floor(nextPos->X), 0.0f, (float)Math_Floor(nextPos->Z));
	Vec3 min, max;
	Vec3_Add(&min, &Blocks.MinBB[block], &horPos);
	Vec3_Add(&max, &Blocks.MaxBB[block], &horPos);
	return nextPos->X >= min.X && nextPos->Z >= min.Z && nextPos->X < max.X && nextPos->Z < max.Z;
}

static BlockID Particle_GetBlock(int x, int y, int z) {
	if (World_Contains(x, y, z)) { return World_GetBlock(x, y, z); }

	if (y >= Env.EdgeHeight) return BLOCK_AIR;
	if (y >= Env_SidesHeight) return Env.EdgeBlock;
	return Env.SidesBlock;
}

static bool Particle_TestY(struct Particle* p, int y, bool topFace, bool throughLiquids) {
	BlockID block;
	Vec3 minBB, maxBB;
	float collideY;
	bool collideVer;

	if (y < 0) {
		p->nextPos.Y = ENTITY_ADJUSTMENT; p->lastPos.Y = ENTITY_ADJUSTMENT;
		p->velocity  = Vec3_Zero();
		particle_hitTerrain = true;
		return false;
	}

	block = Particle_GetBlock((int)p->nextPos.X, y, (int)p->nextPos.Z);
	if (Particle_CanPass(block, throughLiquids)) return true;
	minBB = Blocks.MinBB[block]; maxBB = Blocks.MaxBB[block];

	collideY   = y + (topFace ? maxBB.Y : minBB.Y);
	collideVer = topFace ? (p->nextPos.Y < collideY) : (p->nextPos.Y > collideY);

	if (collideVer && Particle_CollideHor(&p->nextPos, block)) {
		float adjust = topFace ? ENTITY_ADJUSTMENT : -ENTITY_ADJUSTMENT;
		p->lastPos.Y = collideY + adjust;
		p->nextPos.Y = p->lastPos.Y;
		p->velocity  = Vec3_Zero();
		particle_hitTerrain = true;
		return false;
	}
	return true;
}

static bool Particle_PhysicsTick(struct Particle* p, float gravity, bool throughLiquids, double delta) {
	BlockID cur;
	float minY, maxY;
	Vec3 velocity;
	int y, begY, endY;

	p->lastPos = p->nextPos;
	cur  = Particle_GetBlock((int)p->nextPos.X, (int)p->nextPos.Y, (int)p->nextPos.Z);
	minY = Math_Floor(p->nextPos.Y) + Blocks.MinBB[cur].Y;
	maxY = Math_Floor(p->nextPos.Y) + Blocks.MaxBB[cur].Y;

	if (!Particle_CanPass(cur, throughLiquids) && p->nextPos.Y >= minY
		&& p->nextPos.Y < maxY && Particle_CollideHor(&p->nextPos, cur)) {
		return true;
	}

	p->velocity.Y -= gravity * (float)delta;
	begY = Math_Floor(p->nextPos.Y);
	
	Vec3_Mul1(&velocity, &p->velocity, (float)delta * 3.0f);
	Vec3_Add(&p->nextPos, &p->nextPos, &velocity);
	endY = Math_Floor(p->nextPos.Y);

	if (p->velocity.Y > 0.0f) {
		/* don't test block we are already in */
		for (y = begY + 1; y <= endY && Particle_TestY(p, y, false, throughLiquids); y++) {}
	} else {
		for (y = begY; y >= endY && Particle_TestY(p, y, true, throughLiquids); y--) {}
	}

	p->lifetime -= (float)delta;
	return p->lifetime < 0.0f;
}


/*########################################################################################################################*
*-------------------------------------------------------Rain particle-----------------------------------------------------*
*#########################################################################################################################*/
struct RainParticle { struct Particle base; };

static struct RainParticle rain_Particles[PARTICLES_MAX];
static int rain_count;
static TextureRec rain_rec = { 2.0f/128.0f, 14.0f/128.0f, 5.0f/128.0f, 16.0f/128.0f };

static bool RainParticle_Tick(struct RainParticle* p, double delta) {
	particle_hitTerrain = false;
	return Particle_PhysicsTick(&p->base, 3.5f, false, delta) || particle_hitTerrain;
}

static void RainParticle_Render(struct RainParticle* p, float t, VertexP3fT2fC4b* vertices) {
	Vec3 pos;
	Vec2 size;
	PackedCol col;
	int x, y, z;

	Vec3_Lerp(&pos, &p->base.lastPos, &p->base.nextPos, t);
	size.X = (float)p->base.size * 0.015625f; size.Y = size.X;

	x = Math_Floor(pos.X); y = Math_Floor(pos.Y); z = Math_Floor(pos.Z);
	col = World_Contains(x, y, z) ? Lighting_Col(x, y, z) : Env.SunCol;
	Particle_DoRender(&size, &pos, &rain_rec, col, vertices);
}

static void Rain_Render(float t) {
	VertexP3fT2fC4b vertices[PARTICLES_MAX * 4];
	VertexP3fT2fC4b* ptr;
	int i;
	if (!rain_count) return;
	
	ptr = vertices;
	for (i = 0; i < rain_count; i++) {
		RainParticle_Render(&rain_Particles[i], t, ptr);
		ptr += 4;
	}

	Gfx_BindTexture(Particles_TexId);
	Gfx_UpdateDynamicVb_IndexedTris(Particles_VB, vertices, rain_count * 4);
}

static void Rain_RemoveAt(int index) {
	struct RainParticle removed = rain_Particles[index];
	int i;

	for (i = index; i < rain_count - 1; i++) {
		rain_Particles[i] = rain_Particles[i + 1];
	}
	rain_Particles[rain_count - 1] = removed;
	rain_count--;
}

static void Rain_Tick(double delta) {
	int i;
	for (i = 0; i < rain_count; i++) {
		if (RainParticle_Tick(&rain_Particles[i], delta)) {
			Rain_RemoveAt(i); i--;
		}
	}
}


/*########################################################################################################################*
*------------------------------------------------------Terrain particle---------------------------------------------------*
*#########################################################################################################################*/
struct TerrainParticle {
	struct Particle base;
	TextureRec rec;
	TextureLoc texLoc;
	BlockID block;
};

static struct TerrainParticle terrain_particles[PARTICLES_MAX];
static int terrain_count;
static uint16_t terrain_1DCount[ATLAS1D_MAX_ATLASES];
static uint16_t terrain_1DIndices[ATLAS1D_MAX_ATLASES];

static bool TerrainParticle_Tick(struct TerrainParticle* p, double delta) {
	return Particle_PhysicsTick(&p->base, 5.4f, true, delta);
}

static void TerrainParticle_Render(struct TerrainParticle* p, float t, VertexP3fT2fC4b* vertices) {
	PackedCol col = PACKEDCOL_WHITE;
	Vec3 pos;
	Vec2 size;
	int x, y, z;

	Vec3_Lerp(&pos, &p->base.lastPos, &p->base.nextPos, t);
	size.X = (float)p->base.size * 0.015625f; size.Y = size.X;
	
	if (!Blocks.FullBright[p->block]) {
		x = Math_Floor(pos.X); y = Math_Floor(pos.Y); z = Math_Floor(pos.Z);
		col = World_Contains(x, y, z) ? Lighting_Col_XSide(x, y, z) : Env.SunXSide;
	}

	if (Blocks.Tinted[p->block]) {
		PackedCol tintCol = Blocks.FogCol[p->block];
		col.R = (uint8_t)(col.R * tintCol.R / 255);
		col.G = (uint8_t)(col.G * tintCol.G / 255);
		col.B = (uint8_t)(col.B * tintCol.B / 255);
	}
	Particle_DoRender(&size, &pos, &p->rec, col, vertices);
}

static void Terrain_Update1DCounts(void) {
	int i, index;

	for (i = 0; i < ATLAS1D_MAX_ATLASES; i++) {
		terrain_1DCount[i]   = 0;
		terrain_1DIndices[i] = 0;
	}
	for (i = 0; i < terrain_count; i++) {
		index = Atlas1D_Index(terrain_particles[i].texLoc);
		terrain_1DCount[index] += 4;
	}
	for (i = 1; i < Atlas1D.Count; i++) {
		terrain_1DIndices[i] = terrain_1DIndices[i - 1] + terrain_1DCount[i - 1];
	}
}

static void Terrain_Render(float t) {
	VertexP3fT2fC4b vertices[PARTICLES_MAX * 4];
	VertexP3fT2fC4b* ptr;
	int offset = 0;
	int i, index;
	if (!terrain_count) return;

	Terrain_Update1DCounts();
	for (i = 0; i < terrain_count; i++) {
		index = Atlas1D_Index(terrain_particles[i].texLoc);
		ptr   = &vertices[terrain_1DIndices[index]];

		TerrainParticle_Render(&terrain_particles[i], t, ptr);
		terrain_1DIndices[index] += 4;
	}

	Gfx_SetDynamicVbData(Particles_VB, vertices, terrain_count * 4);
	for (i = 0; i < Atlas1D.Count; i++) {
		int partCount = terrain_1DCount[i];
		if (!partCount) continue;

		Gfx_BindTexture(Atlas1D.TexIds[i]);
		Gfx_DrawVb_IndexedTris_Range(partCount, offset);
		offset += partCount;
	}
}

static void Terrain_RemoveAt(int index) {
	struct TerrainParticle removed = terrain_particles[index];
	int i;

	for (i = index; i < terrain_count - 1; i++) {
		terrain_particles[i] = terrain_particles[i + 1];
	}
	terrain_particles[terrain_count - 1] = removed;
	terrain_count--;
}

static void Terrain_Tick(double delta) {
	int i;
	for (i = 0; i < terrain_count; i++) {
		if (TerrainParticle_Tick(&terrain_particles[i], delta)) {
			Terrain_RemoveAt(i); i--;
		}
	}
}


/*########################################################################################################################*
*--------------------------------------------------------Particles--------------------------------------------------------*
*#########################################################################################################################*/
static void Particles_FileChanged(void* obj, struct Stream* stream, const String* name) {
	if (String_CaselessEqualsConst(name, "particles.png")) {
		Game_UpdateTexture(&Particles_TexId, stream, name, NULL);
	}
}

void Particles_Render(double delta, float t) {
	if (!terrain_count && !rain_count) return;
	if (Gfx.LostContext) return;

	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);

	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);
	Terrain_Render(t);
	Rain_Render(t);

	Gfx_SetAlphaTest(false);
	Gfx_SetTexturing(false);
}

void Particles_Tick(struct ScheduledTask* task) {
	Terrain_Tick(task->Interval);
	Rain_Tick(task->Interval);
}

void Particles_BreakBlockEffect(IVec3 coords, BlockID old, BlockID now) {
	struct TerrainParticle* p;
	TextureLoc loc;
	int texIndex;
	TextureRec baseRec, rec;
	Vec3 origin, minBB, maxBB;

	/* texture UV variables */
	float uScale, vScale, maxU2, maxV2;
	int minX, minZ, maxX, maxZ;
	int minU, minV, maxU, maxV;
	int maxUsedU, maxUsedV;
	
	/* per-particle coords */
	float cellX, cellY, cellZ;
	Vec3 cell, pos;
	
	/* per-particle variables */
	Vec3 velocity;
	float life;
	int x, y, z, type;

	if (now != BLOCK_AIR || Blocks.Draw[old] == DRAW_GAS) return;
	IVec3_ToVec3(&origin, &coords);
	loc = Block_Tex(old, FACE_XMIN);
	
	baseRec = Atlas1D_TexRec(loc, 1, &texIndex);
	uScale  = (1.0f/16.0f); vScale = (1.0f/16.0f) * Atlas1D.InvTileSize;

	minBB = Blocks.MinBB[old];    maxBB = Blocks.MaxBB[old];
	minX  = (int)(minBB.X * 16); maxX  = (int)(maxBB.X * 16);
	minZ  = (int)(minBB.Z * 16); maxZ  = (int)(maxBB.Z * 16);

	minU = min(minX, minZ); minV = (int)(16 - maxBB.Y * 16);
	maxU = min(maxX, maxZ); maxV = (int)(16 - minBB.Y * 16);
	/* This way we can avoid creating particles which outside the bounds and need to be clamped */
	maxUsedU = maxU; maxUsedV = maxV;
	if (minU < 12 && maxU > 12) maxUsedU = 12;
	if (minV < 12 && maxV > 12) maxUsedV = 12;

	#define GRID_SIZE 4
	/* gridOffset gives the centre of the cell on a grid */
	#define CELL_CENTRE ((1.0f / GRID_SIZE) * 0.5f)

	maxU2 = baseRec.U1 + maxU * uScale;
	maxV2 = baseRec.V1 + maxV * vScale;
	for (x = 0; x < GRID_SIZE; x++) {
		for (y = 0; y < GRID_SIZE; y++) {
			for (z = 0; z < GRID_SIZE; z++) {

				cellX = (float)x / GRID_SIZE; cellY = (float)y / GRID_SIZE; cellZ = (float)z / GRID_SIZE;
				cell  = Vec3_Create3(CELL_CENTRE + cellX, CELL_CENTRE / 2 + cellY, CELL_CENTRE + cellZ);
				if (cell.X < minBB.X || cell.X > maxBB.X || cell.Y < minBB.Y
					|| cell.Y > maxBB.Y || cell.Z < minBB.Z || cell.Z > maxBB.Z) continue;

				/* centre random offset around [-0.2, 0.2] */
				velocity.X = CELL_CENTRE + (cellX - 0.5f) + (Random_Float(&rnd) * 0.4f - 0.2f);
				velocity.Y = CELL_CENTRE + (cellY - 0.0f) + (Random_Float(&rnd) * 0.4f - 0.2f);
				velocity.Z = CELL_CENTRE + (cellZ - 0.5f) + (Random_Float(&rnd) * 0.4f - 0.2f);

				rec = baseRec;
				rec.U1 = baseRec.U1 + Random_Range(&rnd, minU, maxUsedU) * uScale;
				rec.V1 = baseRec.V1 + Random_Range(&rnd, minV, maxUsedV) * vScale;
				rec.U2 = rec.U1 + 4 * uScale;
				rec.V2 = rec.V1 + 4 * vScale;
				rec.U2 = min(rec.U2, maxU2) - 0.01f * uScale;
				rec.V2 = min(rec.V2, maxV2) - 0.01f * vScale;

				if (terrain_count == PARTICLES_MAX) Terrain_RemoveAt(0);
				p = &terrain_particles[terrain_count++];

				life = 0.3f + Random_Float(&rnd) * 1.2f;
				Vec3_Add(&pos, &origin, &cell);
				Particle_Reset(&p->base, pos, velocity, life);

				p->rec    = rec;
				p->texLoc = loc;
				p->block  = old;
				type = Random_Range(&rnd, 0, 30);
				p->base.size = (uint8_t)(type >= 28 ? 12 : (type >= 25 ? 10 : 8));
			}
		}
	}
}

void Particles_RainSnowEffect(Vec3 pos) {
	struct RainParticle* p;
	Vec3 origin = pos;
	Vec3 offset, velocity;
	int i, type;

	for (i = 0; i < 2; i++) {
		velocity.X = Random_Float(&rnd) * 0.8f - 0.4f; /* [-0.4, 0.4] */
		velocity.Z = Random_Float(&rnd) * 0.8f - 0.4f;
		velocity.Y = Random_Float(&rnd) + 0.4f;

		offset.X = Random_Float(&rnd); /* [0.0, 1.0] */
		offset.Y = Random_Float(&rnd) * 0.1f + 0.01f;
		offset.Z = Random_Float(&rnd);

		if (rain_count == PARTICLES_MAX) Rain_RemoveAt(0);
		p = &rain_Particles[rain_count++];

		Vec3_Add(&pos, &origin, &offset);
		Particle_Reset(&p->base, pos, velocity, 40.0f);

		type = Random_Range(&rnd, 0, 30);
		p->base.size = (uint8_t)(type >= 28 ? 2 : (type >= 25 ? 4 : 3));
	}
}


/*########################################################################################################################*
*---------------------------------------------------Particles component---------------------------------------------------*
*#########################################################################################################################*/
static void Particles_ContextLost(void* obj) {
	Gfx_DeleteVb(&Particles_VB); 
}
static void Particles_ContextRecreated(void* obj) {
	Particles_VB = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, PARTICLES_MAX * 4);
}
static void Particles_BreakBlockEffect_Handler(void* obj, IVec3 coords, BlockID old, BlockID now) {
	Particles_BreakBlockEffect(coords, old, now);
}

static void Particles_Init(void) {
	ScheduledTask_Add(GAME_DEF_TICKS, Particles_Tick);
	Random_SeedFromCurrentTime(&rnd);
	Particles_ContextRecreated(NULL);	

	Event_RegisterBlock(&UserEvents.BlockChanged,   NULL, Particles_BreakBlockEffect_Handler);
	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, Particles_FileChanged);
	Event_RegisterVoid(&GfxEvents.ContextLost,      NULL, Particles_ContextLost);
	Event_RegisterVoid(&GfxEvents.ContextRecreated, NULL, Particles_ContextRecreated);
}

static void Particles_Free(void) {
	Gfx_DeleteTexture(&Particles_TexId);
	Particles_ContextLost(NULL);

	Event_UnregisterBlock(&UserEvents.BlockChanged,   NULL, Particles_BreakBlockEffect_Handler);
	Event_UnregisterEntry(&TextureEvents.FileChanged, NULL, Particles_FileChanged);
	Event_UnregisterVoid(&GfxEvents.ContextLost,      NULL, Particles_ContextLost);
	Event_UnregisterVoid(&GfxEvents.ContextRecreated, NULL, Particles_ContextRecreated);
}

static void Particles_Reset(void) { rain_count = 0; terrain_count = 0; }

struct IGameComponent Particles_Component = {
	Particles_Init,  /* Init  */
	Particles_Free,  /* Free  */
	Particles_Reset, /* Reset */
	Particles_Reset  /* OnNewMap */
};
