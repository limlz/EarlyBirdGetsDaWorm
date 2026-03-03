#include "pch.hpp"

static std::vector<ILLNESSES> RunIllnesses;

void AllAnomalies_Load() {
	Wall_Load();
	Frames_Load();
}

void AllAnomalies_Initialize() {
	Frames_Initialize();
	Lighting_Initialize(7);
	Wall_Initialize();
}

void AllAnomalies_GenerateRun() {
	RunIllnesses.clear();

	std::vector<ILLNESSES> allAnomaliesPool = {
		PARANOIA,
		MANIA,
		DEPRESSION,
		DEMENTIA,
		SCHIZOPHRENIA,
		AIW_SYNDROME,		//Alice in Wonderland Syndrome
		INSOMNIA,
		OCD,
		SCOTOPHOBIA,		//Fear of darkness
		ALL
	};

	std::srand((unsigned)std::time(nullptr));
	std::random_shuffle(allAnomaliesPool.begin(), allAnomaliesPool.end());

	int maxIllnessesPerRun = 7;	

	for (int i = 0; i < maxIllnessesPerRun && i < allAnomaliesPool.size(); ++i) {
		RunIllnesses.push_back(allAnomaliesPool[i]);
	}
}

std::vector<ILLNESSES> AllAnomalies_CurrentRun() {
	if (RunIllnesses.empty()) {
		AllAnomalies_GenerateRun();
	}
	return RunIllnesses;
}