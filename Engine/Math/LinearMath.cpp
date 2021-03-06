#include "LinearMath.h"

Vector3 Vector3::operator*(Quaternion& rh) {
	Vector3 result;
	result.vector = XMVector3Rotate(vector, rh.quaternion);
	return result;
}

Vector3 Vector3::operator*(Matrix4x4& rh) {
	Vector3 result;
	result.vector = XMVector3Transform(vector, rh.matrix);
	return result;
}

Vector3 Vector3::RotateBy(Matrix4x4& rh) {
	Vector3 result;
	result.vector = XMVector3TransformNormal(vector, rh.matrix);
	return result;
}



void Matrix4x4::Tranform(const Vector3& Position, const Quaternion& rotation) {
	//matrix = XMMatrixTransformation(Position.vector, Quaternion().quaternion, Vector3(1, 1, 1).vector, Position.vector, rotation.quaternion, Position.vector);

	XMMATRIX world = XMMatrixRotationQuaternion(rotation.quaternion);
	world.r[3] = XMVectorSetW(Position.vector, 1.0f);
	matrix = world;
}



void Quaternion::FromMatrix(Matrix4x4& m) {
	quaternion = XMQuaternionRotationMatrix(m.matrix);
}

