#pragma once

#include <MAE/Core/Types.h>

#include <string>
#include <MAE/Core/Mat.h>
#include <MAE/Core/Vec.h>

#include <MAE/Core/Deletable.h>

class Shader: public Deletable {
public:
	virtual ~Shader() { }

	inline uint find(const std::string& c) {
		return find(c.c_str());
	}

	inline uint getLocation(const std::string& str) {
		return getLocation(str.c_str());
	}

	virtual uint find(const char* c) = 0;
	virtual void setSampler(uint c, class Texture* sampler) = 0;
	virtual uint getLocation(const char* str) = 0;
	virtual void setFloat(uint c, float f) = 0;
	virtual void setVec2(uint c, const vec2& v) = 0;
	virtual void setVec3(uint c, const vec3& v) = 0;
	virtual void setVec4(uint c, const vec4& v) = 0;
	virtual void setMat3(uint c, const mat3& m) = 0;
	virtual void setMat4(uint c, const mat4& m) = 0;
};
