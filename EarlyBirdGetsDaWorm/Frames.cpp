#include "pch.hpp"

static AEGfxVertexList* frameMesh;
static AEGfxTexture* frames_arr[10] = {nullptr};

void Frames_Load() {

	for (int i = 0; i < 10; i++) {
		std::string filename = "Assets/frames_" + std::to_string(i) + ".png";
		frames_arr[i] = AEGfxTextureLoad(filename.c_str());

		if (frames_arr[i] == nullptr) {
			std::cerr << "Error loading texture: " << filename << std::endl;
		}
	}

}

void Frames_Initialize() {
	frameMesh = CreateSquareMesh(0xFFFFFFFF);
}

void Frames_Update() {
	return;
}

void Frames_Draw(f32 playerX) {
	for (int i = 0; i < 10; i++) {
		DrawTextureMesh(frameMesh, frames_arr[i], 0.0f + 600.0f * i + playerX, 0.0f, 150.0f, 42.0f, 0.5f);
	}
}