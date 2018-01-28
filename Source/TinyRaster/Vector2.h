#pragma once
#include <string>

class Vector2
{
private:
	float m_element[2];

public:
	Vector2();

	Vector2(const Vector2& rhs);

	Vector2(float x, float y);

	~Vector2() { ; }

	float operator [] (const int i) const;
	float& operator [] (const int i);
	Vector2 operator + (const Vector2& rhs) const;
	Vector2 operator - (const Vector2& rhs) const;
	Vector2& operator = (const Vector2& rhs);
	Vector2 operator * (const Vector2& rhs) const;
	Vector2 operator * (float scale) const;

	float Norm()	const;
	float Norm_Sqr() const;
	Vector2 Normalise();

	float DotProduct(const Vector2& rhs) const;
	float CrossProduct(const Vector2& rhs) const;
	
	void SetZero();
	void ReflectAxisY();
	void ReflectAxisX();
	void SwapCoords();


	inline void SetVector(float x, float y)
	{
		m_element[0] = x; m_element[1] = y;
	}	
};


