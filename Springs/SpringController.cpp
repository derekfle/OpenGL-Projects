#include "SpringController.h"

#include <glm/gtc/random.hpp>
#include "ResourceManager.h"

SpringController::SpringController()
{
	// Set the mesh grid to the appropriate size
	MeshGrid.resize(SizeX);
	for (auto& it : MeshGrid)
	{
		it.resize(SizeY);
	}
}


SpringController::~SpringController()
{
	// Release memory
	for (auto& it : Masses)
	{
		delete it;
	}

	for (auto& it : Springs)
	{
		delete it;
	}
}

void SpringController::Initialize()
{
	// Increase screen dimensions and then set the camera location
	SetScreenDimensions(1920, 1080);
	SetCamera(glm::vec3(0.f, 0.f, 10.0f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.0f, 0.f));

	// OpenGL configuration
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	// Initialize the shader to be used for the masses
	RESOURCEMANAGER.LoadShader("basic.vs", "basic.fs", nullptr, "Basic");

	// Setup the springs in the scene
	InitSpring();
	InitSpringWithDampening();
	InitSpringMesh();
}

void SpringController::Update(const GLdouble& dt)
{
	static GLdouble accum = 0;
	accum += dt;
	if (accum < 2.5f) return; // Wait for the scene to fully load

	// Apply external forces to the spring mesh masses
	for (GLint j = 0; j < SizeY; j++)
	{
		glm::vec3 Force = glm::vec3(0.f, -15.f, 25.f * glm::sin(accum));
		for (GLint i = 0; i < SizeX; i++)
		{
			MeshGrid[i][j]->ApplyForce(Force); // Apply external forces
		}
	}

	// Run spring simulations
	for (auto& Spring : Springs)
	{
		Spring->Simulate(static_cast<GLfloat>(dt));
	}

	// Update mass velocities
	for (auto& Mass : Masses)
	{
		Mass->Update(static_cast<GLfloat>(dt));
	}
}

void SpringController::ProcessInput(const GLdouble& dt)
{
}

void SpringController::Render()
{
	// Render all the masses on the screen, change the color of mass depending on whether it is fixed or not
	for (auto& Mass : Masses)
	{
		if (Mass->IsFixedPoint())
		{
			RESOURCEMANAGER.GetShader("Basic").SetVector3f("inColor", glm::vec3(0.f, 0.75f, 1.f), true);
		}
		else
		{
			RESOURCEMANAGER.GetShader("Basic").SetVector3f("inColor", glm::vec3(1.0f, 0.5f, 0.2f), true);
		}
		Mass->Draw();
	}
}

void SpringController::InitSpring()
{
	PointMass* M1 = new PointMass(1, true);
	M1->SetShader("Basic");
	M1->SetTranslation(glm::vec3(-6.f, 4.f, 0.f));
	Masses.push_back(M1);

	PointMass* M2 = new PointMass(10);
	M2->SetShader("Basic");
	M2->SetTranslation(glm::vec3(-6.f, -4.f, 0.f));
	Masses.push_back(M2);

	Spring* S1 = new Spring(M1, M2, 10.f, glm::length(Masses[0]->GetPosition() - Masses[1]->GetPosition()) / 2.f, 0.05f);
	Springs.push_back(S1);
}

void SpringController::InitSpringWithDampening()
{
	PointMass* M1 = new PointMass(1, true);
	M1->SetShader("Basic");
	M1->SetTranslation(glm::vec3(-4.f, 4.f, 0.f));
	Masses.push_back(M1);

	PointMass* M2 = new PointMass(20);
	M2->SetShader("Basic");
	M2->SetTranslation(glm::vec3(-4.f, -4.f, 0.f));
	Masses.push_back(M2);

	Spring* S1 = new Spring(M1, M2, 10.f, glm::length(Masses[0]->GetPosition() - Masses[1]->GetPosition()) / 2.f, 1.f);
	Springs.push_back(S1);
}

void SpringController::InitSpringMesh()
{
	for (GLint i = 0; i < SizeX; i++)
	{
		for (GLint j = -SizeY / 2; j < SizeY / 2; j++)
		{
			// Make the top two rows static
			PointMass* M = (j >= SizeY / 2 - 2) ? new PointMass(1, true) : new PointMass(10);
			M->SetShader("Basic");

			M->SetTranslation(glm::vec3(i * 0.05f + 3.f, j * 0.05f + 0.5f, 0.f));
			M->SetScale(glm::vec3(0.025f, 0.025f, 0.025f));

			Masses.push_back(M);

			MeshGrid[i][j + SizeY / 2] = M;
		}
	}

	const GLfloat Dampening = 1.0f;
	const GLfloat SpringConstant = 10000.f;
	for (GLint i = 0; i < SizeX; i++)
	{
		for (GLint j = 0; j < SizeY; j++)
		{
			PointMass* M = MeshGrid[i][j];

			// Structural
			if (i != 0)
			{
				Spring* S = new Spring(MeshGrid[i][j], MeshGrid[i - 1][j], SpringConstant, glm::length(MeshGrid[i][j]->GetPosition() - MeshGrid[i - 1][j]->GetPosition()), Dampening);
				Springs.push_back(S);
			}

			if (j != 0)
			{
				Spring* S = new Spring(MeshGrid[i][j], MeshGrid[i][j - 1], SpringConstant, glm::length(MeshGrid[i][j - 1]->GetPosition() - MeshGrid[i][j]->GetPosition()), Dampening);
				Springs.push_back(S);
			}

			// Shear
			if (i != 0 && j != SizeY - 1)
			{
				Spring* S = new Spring(MeshGrid[i][j], MeshGrid[i - 1][j + 1], SpringConstant, glm::length(MeshGrid[i][j]->GetPosition() - MeshGrid[i - 1][j + 1]->GetPosition()), Dampening);
				Springs.push_back(S);
			}

			if (i != 0 && j != 0)
			{
				Spring* S = new Spring(MeshGrid[i][j], MeshGrid[i - 1][j - 1], SpringConstant, glm::length(MeshGrid[i][j]->GetPosition() - MeshGrid[i - 1][j - 1]->GetPosition()), Dampening);
				Springs.push_back(S);
			}

			// Bend
			if (i > 1)
			{
				Spring* S = new Spring(MeshGrid[i][j], MeshGrid[i - 2][j], SpringConstant, glm::length(MeshGrid[i][j]->GetPosition() - MeshGrid[i - 2][j]->GetPosition()), Dampening);
				Springs.push_back(S);
			}

			if (j > 1)
			{
				Spring* S = new Spring(MeshGrid[i][j], MeshGrid[i][j - 2], SpringConstant, glm::length(MeshGrid[i][j]->GetPosition() - MeshGrid[i][j - 2]->GetPosition()), Dampening);
				Springs.push_back(S);
			}
		}
	}
}