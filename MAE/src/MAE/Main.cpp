#include <MAE/Main.h>
#include <MAE/Rendering/ModelMD2.h>
#include <MAE/Rendering/VertexBuffer.h>
#include <MAE/Rendering/VertexDecl.h>
#include <MAE/Rendering/ModelMPM.h>
#include <MAE/Rendering/ModelX.h>
#include <MAE/Rendering/ParticleSystem.h>
#include <MAE/MainImpl.h>
#include <MAE/Rendering/Resources/SurfaceImpl.h>
#include <MAE/Rendering/Resources/TextureImpl.h>
#include <MAE/Rendering/Resources/ShaderImpl.h>
#include <MAE/Rendering/Scene/SceneImpl.h>

Main* mainObj = 0;

void MainImpl::release() {
	::delete this;
}

MainImpl::MainImpl(LPDIRECT3DDEVICE9 d3ddev)
{
	d3ddev->AddRef();
	d3ddev->GetDirect3D(&d3d);
	d3d->AddRef();

	this->d3ddev = d3ddev;
}

MainImpl::~MainImpl() {
	assert(("Some MD2 Models weren't freed", MD2Models.size() == 0));
	assert(("Some MPM Models weren't freed", MPMModels.size() == 0));
	assert(("Some Lights weren't freed", Light.size() == 0));
	assert(("Some Materials weren't freed", Material.size() == 0));
	assert(("Some Vertex Declarations weren't freed", VertexDeclarations.size() == 0));
	assert(("Some Vertex Buffers weren't freed", VertexBuffers.size() == 0));
	assert(("Some X Models weren't freed", XModels.size() == 0));
	assert(("Some Particel Systems weren't freed", ParticleSys.size() == 0));
	assert(("Some Shaders weren't freed", shaders.size() == 0));
	assert(("Some Surfaces weren't freed", surfaces.size() == 0));
	assert(("Some Texture weren't freed", textures.size() == 0));

	if (VertexDeclarationParticle != 0) {
		VertexDeclarationParticle->Release();
		VertexDeclarationParticle = 0;
	}

	if (d3ddev != 0)
		d3ddev->Release();

	if (d3d != 0)
		d3d->Release();

	d3ddev = 0;
	d3d    = 0;
}

ErrorCode MainImpl::checkFormat(D3DFORMAT adapdterFmt, uint usage, D3DRESOURCETYPE type, D3DFORMAT fmt, bool& exists) {
	exists = SUCCEEDED(d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapdterFmt, usage, type, fmt));

	return ErrorOk;
}

ErrorCode MainImpl::setError(ErrorCode code)
{
	for (auto i : functions)
		i(code);

	return errCode = code;
}

ErrorCode MainImpl::getError()
{
	return errCode;
}

ErrorCode MainImpl::createScene(Scene*& scene) {
	SceneImpl* s = ::new(std::nothrow) SceneImpl(this);

	if (s == 0)
		return setError(ErrorMemory);

	scenes.add(s);
	scene = (Scene*) s;

	return ErrorOk;
}

ErrorCode MainImpl::createSurface(Surface*& surf)
{
	SurfaceImpl* s = ::new(std::nothrow) SurfaceImpl(this);

	if (s == 0)
		return this->setError(ErrorMemory);

	surfaces.add(s);
	surf = s;

	return ErrorOk;
}

void MainImpl::removeSurface(const Surface* surf)
{
	surfaces.remove((SurfaceImpl*) surf);
}

ErrorCode MainImpl::createTexture(Texture*& tex)
{
	TextureImpl* t = ::new(std::nothrow) TextureImpl(this);

	if (t == 0)
		return this->setError(ErrorMemory);

	textures.add(t);
	tex = t;

	return ErrorOk;
}

void MainImpl::removeTexture(const Texture* tex)
{
	textures.remove((TextureImpl*) tex);
}

ErrorCode MainImpl::createShader(Shader*& shd)
{
	ShaderImpl* s = ::new(std::nothrow) ShaderImpl(this);

	if (s == 0)
		return setError(ErrorMemory);

	shaders.add(s);
	shd = s;

	return ErrorOk;
}

void MainImpl::removeShader(const Shader* shd)
{
	shaders.remove((ShaderImpl*) shd);
}

ErrorCode MainImpl::getRenderTarget(uint ind, class Surface*& surf)
{
	LPDIRECT3DSURFACE9 s;

	if (FAILED(d3ddev->GetRenderTarget(ind, &s)))
		return setError(ErrorD3D9);

	ErrorCode ret = createSurface(surf);

	if (ret == ErrorOk)
		ret = surf->createFromPointer(s);

	s->Release();

	return ret;
}

ErrorCode MainImpl::resetRenderTarget(uint ind)
{
	if (FAILED(d3ddev->SetRenderTarget(ind, nullptr)))
		return setError(ErrorD3D9);

	return ErrorOk;
}

ErrorCode MainImpl::setRenderTarget(uint ind, class Surface* surf)
{
	LPDIRECT3DSURFACE9 s;
	
	ErrorCode ret = surf->getSurf(s);

	if (ret != ErrorOk)
		return ret;

	HRESULT res = d3ddev->SetRenderTarget(ind, s);

	s->Release();

	if (FAILED(res))
		return setError(ErrorD3D9);

	return ErrorOk;
}

ErrorCode MainImpl::setTexture(uint stage, class Texture* tex)
{
	LPDIRECT3DTEXTURE9 t;

	ErrorCode ret = tex->getTexture(t);

	if (ret != ErrorOk)
		return ret;
	
	HRESULT res = d3ddev->SetTexture(stage, t);

	t->Release();

	if (FAILED(res))
		return setError(ErrorD3D9);

	return ErrorOk;
}

ErrorCode MainImpl::resetTexture(uint stage)
{
	if (FAILED(d3ddev->SetTexture(stage, 0)))
		return setError(ErrorD3D9);

	return ErrorOk;
}

ErrorCode MainImpl::setShader(Shader* shd)
{
	LPDIRECT3DVERTEXSHADER9 vshd;
	LPDIRECT3DPIXELSHADER9 pshd;

	ErrorCode ret;
	
	if ((ret = shd->getVertexShader(vshd)) != ErrorOk)
		return ret;

	if ((ret = shd->getPixelShader(pshd)) != ErrorOk)
	{
		vshd->Release();
		return ret;
	}

	d3ddev->SetVertexShader(vshd);
	d3ddev->SetPixelShader(pshd);

	vshd->Release();
	pshd->Release();

	return ErrorOk;
}

ErrorCode MainImpl::resetShader()
{
	d3ddev->SetVertexShader(0);
	d3ddev->SetPixelShader(0);

	return ErrorOk;
}

ErrorCode MainImpl::onError(void(*func)(ErrorCode))
{
	if (std::find(functions.begin(), functions.end(), func) == functions.end())
		return ErrorOk;

	functions.push_back(func);

	return ErrorOk;
}

ErrorCode MainImpl::unregisterErrorFunction(void(*func)(ErrorCode))
{
	functions.remove_if([func](void(*f)(ErrorCode)) { return func == f; });

	return ErrorOk;
}

void MainImpl::removeScene(const class Scene* scene) {
	scenes.remove((SceneImpl*) scene);
}

Main* MainCreate(LPDIRECT3DDEVICE9 device)
{
	// return new(std::nothrow) MainImpl(device);
	return (mainObj = ::new(std::nothrow) MainImpl(device));
}
