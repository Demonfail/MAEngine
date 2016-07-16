#pragma once

/**
* Includes
*/
#include <MAE/Core/Types.h>
#include <MAE/Core/Math.h>
#include <MAE/Rendering/Particle.h>
#include <MAE/Rendering/ParticleEmitter.h>
#include <MAE/Rendering/ParticleModifier.h>
#include <MAE/Rendering/Resources/Texture.h>

class ParticleSystem {
public:
	ParticleSystem(class Renderer* renderer);
	~ParticleSystem();

	void createEmitter();

	ParticleEmitter* getEmitter();

	void update(uint time);
	void render();

	void setTexture(Texture* tex);

	uint getParticleCount();

	void setMaxParticleCount(uint max);
private:
	class Renderer* renderer;

	std::vector<ParticleModifier*> psMods;
	ParticleEmitter*   psEmitter = NULL;

	std::vector<Particle> psBuffer;

	uint psMaxParticleCount;

	Texture* texture = 0;

	//D3D...
	LPDIRECT3DVERTEXBUFFER9 psVertexBuffer = 0;

	LPDIRECT3DVERTEXDECLARATION9 decl;
};