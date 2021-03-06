#include "SpringController.h"

#include <glm/gtc/random.hpp>
#include "ResourceManager.h"

#define ScreenX 1920
#define ScreenY 1080

SpringController::SpringController() :
	bIsCameraMoving(false),
	MouseSensitivity(0.0005f)
{
	// Set the mesh grid to the appropriate size
	MeshGrid.resize(SizeX);
	for (auto& it : MeshGrid)
	{
		it.resize(SizeY);
	}

	// Build the spline
	Spline.AddPoint(glm::vec3(0.f, 0.f, 12.f));
	Spline.AddPoint(glm::vec3(-12.f, 12.f, 0.f));
	Spline.AddPoint(glm::vec3(0.f, 0.f, -12.f));
	Spline.AddPoint(glm::vec3(12.f, -12.f, 0.f));
	Spline.AddPoint(glm::vec3(0.f, 0.f, 12.f));
	Spline.AddPoint(glm::vec3(0.f, 0.f, -12.f));
	Spline.AddPoint(glm::vec3(-12.f, 0.f, 0.f));
	Spline.AddPoint(glm::vec3(0.f, 0.f, 12.f));
	Spline.AddPoint(glm::vec3(12.f, 5.f, 0.f));
	Spline.AddPoint(glm::vec3(0.f, 5.f, -12.f));
	Spline.AddPoint(glm::vec3(0.f, 0.f, 12.f));
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

	for (auto& it : AngSprings)
	{
		delete it;
	}
}

void SpringController::Initialize()
{
	GameController::Initialize();

	// Hide the mouse cursor
	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	// Increase screen dimensions and then set the camera location
	SetScreenDimensions(ScreenX, ScreenY);
	Camera->SetPosition(glm::vec3(0.f, 0.f, 10.f));
	Camera->SetFocus(glm::vec3(0.f, 0.f, 0.f));
	Camera->SetWorldUp(glm::vec3(0.f, 1.f, 0.f));
	Camera->UpdateView();

	// OpenGL configuration
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	// Initialize the shader to be used for the masses
	RESOURCEMANAGER.LoadShader("phong.vs", "phong.fs", nullptr, "Phong");

	// Set the directional light direction and color
	RESOURCEMANAGER.SetLightDirection(glm::vec3(-1.f, 0.5f, 1.f));
	RESOURCEMANAGER.SetLightColor(glm::vec3(1.f, 1.f, 1.f));

	// Setup the springs in the scene
	InitSpring();
	InitSpringWithDampening();
	InitSpringMesh();
	InitAngularSpring();
}

void SpringController::Update(const GLfloat& dt)
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
		Spring->Simulate(dt);
	}

	for (auto& AngSpring : AngSprings)
	{
		AngSpring->Simulate(dt);
	}

	// Update mass velocities
	for (auto& Mass : Masses)
	{
		Mass->Update(dt);
	}

	// Update camera
	if (bIsCameraMoving)
	{
		UpdateCamera(dt);
	}
}

void SpringController::ProcessInput(const GLint& Key, const GLint& Action, const GLint& Mode)
{
	if (Key == GLFW_KEY_SPACE && Action == GLFW_RELEASE)
	{
		bIsCameraMoving = !bIsCameraMoving;
	}
}

void SpringController::ProcessMouseMove(const GLdouble& dX, const GLdouble& dY)
{
	static GLint normalX = ScreenX / 2;
	static GLint normalY = ScreenY / 2;

	GLboolean bDidUpdate = false;
	if (dX != normalX || dY != normalY)
	{
		Camera->RotateByMouse(static_cast<GLfloat>(dX - normalX) * MouseSensitivity, static_cast<GLfloat>(dY - normalY) * MouseSensitivity);
		Camera->UpdateView();

		bDidUpdate = true;
	}

	if (bDidUpdate)
	{
		glfwSetCursorPos(Window, normalX, normalY);
	}
}

void SpringController::Render()
{
	for (auto& AngSpring : AngSprings)
	{
		AngSpring->Draw();
	}

	// Render all the masses on the screen, change the color of mass depending on whether it is fixed or not
	for (auto& Mass : Masses)
	{
		if (Mass->IsFixedPoint())
		{
			RESOURCEMANAGER.GetShader("Phong").SetVector3f("Color", glm::vec3(0.f, 0.75f, 1.f), true);
		}
		else
		{
			RESOURCEMANAGER.GetShader("Phong").SetVector3f("Color", glm::vec3(1.0f, 0.5f, 0.2f), true);
		}
		Mass->Draw();
	}
}

void SpringController::InitSpring()
{
	PointMass* M1 = new PointMass(1, true);
	M1->SetShader("Phong");
	M1->SetTranslation(glm::vec3(-6.f, 4.f, 0.f));
	Masses.push_back(M1);

	PointMass* M2 = new PointMass(10);
	M2->SetShader("Phong");
	M2->SetTranslation(glm::vec3(-6.f, -4.f, 0.f));
	Masses.push_back(M2);

	Spring* S1 = new Spring(M1, M2, 10.f, glm::length(Masses[0]->GetPosition() - Masses[1]->GetPosition()) / 2.f, 0.05f);
	Springs.push_back(S1);
}

void SpringController::InitSpringWithDampening()
{
	PointMass* M1 = new PointMass(1, true);
	M1->SetShader("Phong");
	M1->SetTranslation(glm::vec3(-4.f, 4.f, 0.f));
	Masses.push_back(M1);

	PointMass* M2 = new PointMass(20);
	M2->SetShader("Phong");
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
			M->SetShader("Phong");

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

void SpringController::InitAngularSpring()
{
	AngularSpring* ASpring = new AngularSpring("Phong", 3.f, 10.f);
	ASpring->SetTranslation(glm::vec3(-2.f, -3.f, 0.f));
	ASpring->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
	ASpring->Init();

	AngSprings.push_back(ASpring);

	ASpring = new AngularSpring("Phong", 3.f, 10.f, 0.95f);
	ASpring->SetTranslation(glm::vec3(-2.f, 1.f, 0.f));
	ASpring->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
	ASpring->Init();

	AngSprings.push_back(ASpring);
}

void SpringController::UpdateCamera(const GLfloat& dt)
{
	Camera->SetPosition(Spline.GetInterpolatedPoint(dt));
	Camera->UpdateView();
}