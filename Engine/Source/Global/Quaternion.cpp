#include "pch.h"
#include "Global/Quaternion.h"

FQuaternion FQuaternion::FromAxisAngle(const FVector& Axis, float AngleRad)
{
	FVector N = Axis;
	N.Normalize();
	float s = sinf(AngleRad * 0.5f);
	return FQuaternion(
		N.X * s,
		N.Y * s,
		N.Z * s,
		cosf(AngleRad * 0.5f)
	);
}

FQuaternion FQuaternion::FromEuler(const FVector& EulerDeg)
{
	FVector Radians = FVector::GetDegreeToRadian(EulerDeg);

	float cx = cosf(Radians.X * 0.5f);
	float sx = sinf(Radians.X * 0.5f);
	float cy = cosf(Radians.Y * 0.5f);
	float sy = sinf(Radians.Y * 0.5f);
	float cz = cosf(Radians.Z * 0.5f);
	float sz = sinf(Radians.Z * 0.5f);

	// Yaw-Pitch-Roll (Z, Y, X)
	return FQuaternion(
		sx * cy * cz - cx * sy * sz, // X
		cx * sy * cz + sx * cy * sz, // Y
		cx * cy * sz - sx * sy * cz, // Z
		cx * cy * cz + sx * sy * sz  // W
	);
}

FVector FQuaternion::ToEuler() const
{
	FVector Euler;

	// Roll (X)
	float sinr_cosp = 2.0f * (W * X + Y * Z);
	float cosr_cosp = 1.0f - 2.0f * (X * X + Y * Y);
	Euler.X = atan2f(sinr_cosp, cosr_cosp);

	// Pitch (Y)
	float sinp = 2.0f * (W * Y - Z * X);
	if (fabs(sinp) >= 1)
		Euler.Y = copysignf(PI / 2, sinp); // 90도 고정
	else
		Euler.Y = asinf(sinp);

	// Yaw (Z)
	float siny_cosp = 2.0f * (W * Z + X * Y);
	float cosy_cosp = 1.0f - 2.0f * (Y * Y + Z * Z);
	Euler.Z = atan2f(siny_cosp, cosy_cosp);

	return FVector::GetRadianToDegree(Euler);
}

FQuaternion FQuaternion::operator*(const FQuaternion& Q) const
{
	return FQuaternion(
		W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z
	);
}

void FQuaternion::Normalize()
{
	float mag = sqrtf(X * X + Y * Y + Z * Z + W * W);
	if (mag > 0.0001f)
	{
		X /= mag;
		Y /= mag;
		Z /= mag;
		W /= mag;
	}
}

FVector FQuaternion::RotateVector(const FQuaternion& q, const FVector& v)
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = q * p * q.Inverse();
	return FVector(r.X, r.Y, r.Z);
}

FVector FQuaternion::RotateVector(const FVector& v) const
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = (*this) * p * this->Inverse();
	return FVector(r.X, r.Y, r.Z);
}

FQuaternion FQuaternion::FromMatrix(const FMatrix& M)
{
    FQuaternion Q;
    float Trace = M.Data[0][0] + M.Data[1][1] + M.Data[2][2];
    if (Trace > 0)
    {
        float S = sqrtf(Trace + 1.0f) * 2.0f;
        Q.W = 0.25f * S;
        Q.X = (M.Data[2][1] - M.Data[1][2]) / S;
        Q.Y = (M.Data[0][2] - M.Data[2][0]) / S;
        Q.Z = (M.Data[1][0] - M.Data[0][1]) / S;
    }
    else if ((M.Data[0][0] > M.Data[1][1]) && (M.Data[0][0] > M.Data[2][2]))
    {
        float S = sqrtf(1.0f + M.Data[0][0] - M.Data[1][1] - M.Data[2][2]) * 2.0f;
        Q.W = (M.Data[2][1] - M.Data[1][2]) / S;
        Q.X = 0.25f * S;
        Q.Y = (M.Data[0][1] + M.Data[1][0]) / S;
        Q.Z = (M.Data[0][2] + M.Data[2][0]) / S;
    }
    else if (M.Data[1][1] > M.Data[2][2])
    {
        float S = sqrtf(1.0f + M.Data[1][1] - M.Data[0][0] - M.Data[2][2]) * 2.0f;
        Q.W = (M.Data[0][2] - M.Data[2][0]) / S;
        Q.X = (M.Data[0][1] + M.Data[1][0]) / S;
        Q.Y = 0.25f * S;
        Q.Z = (M.Data[1][2] + M.Data[2][1]) / S;
    }
    else
    {
        float S = sqrtf(1.0f + M.Data[2][2] - M.Data[0][0] - M.Data[1][1]) * 2.0f;
        Q.W = (M.Data[1][0] - M.Data[0][1]) / S;
        Q.X = (M.Data[0][2] + M.Data[2][0]) / S;
        Q.Y = (M.Data[1][2] + M.Data[2][1]) / S;
        Q.Z = 0.25f * S;
    }
    return Q;
}
