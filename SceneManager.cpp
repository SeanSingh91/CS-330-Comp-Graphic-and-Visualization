///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method defines the Phong material properties -
 *  ambient, diffuse, specular, and shininess - associated
 *  with each surface tag used in the scene. Matching a
 *  material to every texture tag means each object responds
 *  to light the way its real-world counterpart would: the
 *  wood desk gets a soft sheen, the hardcover jackets get a
 *  tighter highlight, and the paper and cloth surfaces stay
 *  mostly matte.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL material;

	// wood desk surface - finished wood has a gentle, broad
	// sheen rather than a sharp highlight
	material.tag = "wood";
	material.ambientColor = glm::vec3(0.50f, 0.38f, 0.26f);
	material.ambientStrength = 0.18f;
	material.diffuseColor = glm::vec3(0.30f, 0.40f, 0.30f);
	material.specularColor = glm::vec3(0.35f, 0.35f, 0.30f);
	material.shininess = 12.0f;
	m_objectMaterials.push_back(material);

	// black leather hardcover (Book 4) - smoother and glossier,
	// so it picks up a brighter, tighter specular highlight
	material.tag = "leather";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.25f;
	material.diffuseColor = glm::vec3(0.12f, 0.12f, 0.12f);
	material.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	material.shininess = 40.0f;
	m_objectMaterials.push_back(material);

	// lighter, worn paperback cover (Book 3)
	material.tag = "leather2";
	material.ambientColor = glm::vec3(0.18f, 0.18f, 0.17f);
	material.ambientStrength = 0.25f;
	material.diffuseColor = glm::vec3(0.45f, 0.45f, 0.42f);
	material.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	material.shininess = 30.0f;
	m_objectMaterials.push_back(material);

	// cloth-bound hardcover (Book 1 top/bottom) - matte weave,
	// so light diffuses broadly with almost no specular pop
	material.tag = "fabric";
	material.ambientColor = glm::vec3(0.15f, 0.15f, 0.16f);
	material.ambientStrength = 0.3f;
	material.diffuseColor = glm::vec3(0.4f, 0.4f, 0.42f);
	material.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.shininess = 4.0f;
	m_objectMaterials.push_back(material);

	// lighter interior fabric/pages (Book 1 middle, Book 6 pages)
	material.tag = "fabric2";
	material.ambientColor = glm::vec3(0.18f, 0.17f, 0.15f);
	material.ambientStrength = 0.3f;
	material.diffuseColor = glm::vec3(0.5f, 0.48f, 0.44f);
	material.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.shininess = 4.0f;
	m_objectMaterials.push_back(material);

	// plain white paperback cover (Book 2) - very matte
	material.tag = "paper";
	material.ambientColor = glm::vec3(0.2f, 0.2f, 0.19f);
	material.ambientStrength = 0.35f;
	material.diffuseColor = glm::vec3(0.55f, 0.55f, 0.52f);
	material.specularColor = glm::vec3(0.03f, 0.03f, 0.03f);
	material.shininess = 2.0f;
	m_objectMaterials.push_back(material);

	// stacked page edges (Book 4 middle) - same matte family as paper
	material.tag = "pages";
	material.ambientColor = glm::vec3(0.2f, 0.2f, 0.19f);
	material.ambientStrength = 0.35f;
	material.diffuseColor = glm::vec3(0.55f, 0.55f, 0.5f);
	material.specularColor = glm::vec3(0.03f, 0.03f, 0.03f);
	material.shininess = 2.0f;
	m_objectMaterials.push_back(material);

	// bright white book block (Book 2 pages)
	material.tag = "white";
	material.ambientColor = glm::vec3(0.25f, 0.25f, 0.24f);
	material.ambientStrength = 0.4f;
	material.diffuseColor = glm::vec3(0.7f, 0.7f, 0.68f);
	material.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.shininess = 3.0f;
	m_objectMaterials.push_back(material);

	// light cream cover/pages (Book 5) - textile-like, low sheen
	material.tag = "carpet";
	material.ambientColor = glm::vec3(0.15f, 0.14f, 0.12f);
	material.ambientStrength = 0.3f;
	material.diffuseColor = glm::vec3(0.4f, 0.38f, 0.32f);
	material.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	material.shininess = 2.0f;
	m_objectMaterials.push_back(material);

	// rough paper cover/spine accents (Book 3 top/bottom/side)
	material.tag = "rough";
	material.ambientColor = glm::vec3(0.12f, 0.12f, 0.11f);
	material.ambientStrength = 0.25f;
	material.diffuseColor = glm::vec3(0.32f, 0.32f, 0.3f);
	material.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	material.shininess = 2.0f;
	m_objectMaterials.push_back(material);

	// dark notebook cover (Book 6) - a bit glossier than cloth,
	// closer to a laminated hardcover finish
	material.tag = "stone";
	material.ambientColor = glm::vec3(0.08f, 0.08f, 0.09f);
	material.ambientStrength = 0.25f;
	material.diffuseColor = glm::vec3(0.2f, 0.2f, 0.22f);
	material.specularColor = glm::vec3(0.25f, 0.25f, 0.25f);
	material.shininess = 25.0f;
	m_objectMaterials.push_back(material);

	// glazed ceramic mug - smooth surface with a moderate,
	// rounded highlight, glossier than the book covers but not
	// as sharp as glass
	material.tag = "ceramic";
	material.ambientColor = glm::vec3(0.20f, 0.20f, 0.22f);
	material.ambientStrength = 0.30f;
	material.diffuseColor = glm::vec3(0.55f, 0.55f, 0.60f);
	material.specularColor = glm::vec3(0.25f, 0.25f, 0.27f);
	material.shininess = 10.0f;
	m_objectMaterials.push_back(material);

	// glass paperweight - very glossy with a small, bright,
	// tightly focused highlight
	material.tag = "glass";
	material.ambientColor = glm::vec3(0.10f, 0.12f, 0.14f);
	material.ambientStrength = 0.20f;
	material.diffuseColor = glm::vec3(0.25f, 0.35f, 0.45f);
	material.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	material.shininess = 90.0f;
	m_objectMaterials.push_back(material);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method configures the light sources that illuminate
 *  the scene, using the Phong lighting model (ambient +
 *  diffuse + specular). Two point lights are used:
 *
 *    Light 0 - the KEY light. Positioned high and to the
 *              front-left of the book stack, this is the
 *              scene's main light source and does most of
 *              the work lighting the covers and the desk.
 *    Light 1 - the FILL light. Positioned on the opposite
 *              side at a much lower intensity, this exists
 *              only to lift the side of the stack facing
 *              away from the key light so it never falls
 *              into complete shadow
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// turn on the lighting calculations in the fragment shader -
	// without this flag the shader just renders flat texture/color
	// with no shading at all
	m_pShaderManager->setIntValue(g_UseLightingName, true);

	// LIGHT 0 (key light) - warm white, positioned above and in
	// front of the stack like an overhead desk lamp
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(-4.0f, 8.0f, 6.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.14f, 0.14f, 0.13f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.61f, 0.56f, 0.48f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.42f, 0.42f, 0.42f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.20f);

	// LIGHT 1 (fill light) - cooler and much dimmer, on the
	// opposite side of the stack so no surface is left in
	// total darkness
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-6.0f, 5.0f, -20.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.07f, 0.07f, 0.08f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.27f, 0.29f, 0.34f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.15f, 0.15f, 0.15f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.08f);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 * 
 *  *  Three image-based textures are loaded for this milestone:
 *    "wood"    - a plank/wood-grain image tiled across the
 *                desk plane so individual boards read clearly
 *                instead of one big stretched image
 *    "leather" - applied to the front/back covers of Book 4,
 *                the multi-shape (complex) object in the scene
 *    "pages"   - a paper/page-edge image applied to the pages
 *                block sandwiched between Book 4's covers, and
 *                repeated vertically to suggest many stacked sheets
 *
 *  All three source images are free, CC0-licensed PBR textures
 *  from ambientCG.com (Wood039 / Planks010, Leather007, Paper001),
 *  downloaded and stored locally under Utilities/textures/.
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();

	// additional primitives needed for the coffee mug, pencil cup,
	// pencils, and paperweight added for the final project
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();

	// load the image files used to texture the scene and register
	// each one under a short, descriptive tag so RenderScene() can
	// look it up later without caring about the actual file path
	bool bReturn = true;
	bReturn &= CreateGLTexture("textures/wood.jpg", "wood");
	bReturn &= CreateGLTexture("textures/leather.jpg", "leather");
	bReturn &= CreateGLTexture("textures/pages.jpg", "pages");
	bReturn &= CreateGLTexture("textures/carpet.jpg", "carpet");
	bReturn &= CreateGLTexture("textures/paper.jpg", "paper");
	bReturn &= CreateGLTexture("textures/stone.png", "stone");
	bReturn &= CreateGLTexture("textures/fabric.jpg", "fabric");
	bReturn &= CreateGLTexture("textures/rough.jpg", "rough");
	bReturn &= CreateGLTexture("textures/white.jpg", "white");
	bReturn &= CreateGLTexture("textures/fabric2.jpg", "fabric2");
	bReturn &= CreateGLTexture("textures/leather2.jpg", "leather2");
	bReturn &= CreateGLTexture("textures/ceramic.jpg", "ceramic");

	if (bReturn == false)
	{
		std::cout << "WARNING: one or more textures failed to load - check the file paths above." << std::endl;
	}

	// bind all successfully loaded textures into their OpenGL texture
	// units now, once, rather than re-binding on every draw call
	BindGLTextures();

	// register the material (ambient/diffuse/specular/shininess) that
	// goes with each texture tag, so the shader knows how each surface
	// should respond once lighting is turned on
	DefineObjectMaterials();

	// turn on the scene's light sources - this has to happen after the
	// materials are defined so every SetShaderMaterial() call below
	// draws with a properly lit surface instead of a flat unlit one
	SetupSceneLights();
}

/***********************************************************
 *  RenderMug()
 *
 *  Draws a coffee mug from three primitives - a cylinder body
 *  and a torus handle - and treats them as one reusable object.
 *  Bundling both primitives behind a single call means the mug
 *  can be moved, resized, or duplicated anywhere in the scene
 *  by changing just three parameters, instead of hand-editing
 *  two separate sets of transform values every time.
 *
 *  positionXYZ       - where the base of the mug sits (world space)
 *  rotationYDegrees  - which direction the handle points
 *  scaleFactor        - uniform size multiplier for the whole mug
 ***********************************************************/
void SceneManager::RenderMug(
	glm::vec3 positionXYZ,
	float rotationYDegrees,
	float scaleFactor)
{
	glm::vec3 scaleXYZ;
	glm::vec3 handlePosition;

	// MUG BODY - cylinder, base resting on the desk surface
	scaleXYZ = glm::vec3(0.55f, 0.75f, 0.55f) * scaleFactor;
	SetTransformations(scaleXYZ, 0.0f, rotationYDegrees, 0.0f, positionXYZ);
	SetShaderTexture("ceramic");
	SetShaderMaterial("ceramic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// COFFEE - a short, slightly-smaller-radius cylinder sitting just
	// below the rim so it reads as liquid inside the mug rather than
	// poking through the walls. Since the mug body isn't a hollow
	// shell, this is just a second solid cylinder layered on top,
	// sized and positioned to look like a filled surface.
	glm::vec3 coffeePosition = positionXYZ + glm::vec3(0.0f, 0.712f * scaleFactor, 0.0f);
	scaleXYZ = glm::vec3(0.46f, 0.04f, 0.46f) * scaleFactor;
	SetTransformations(scaleXYZ, 0.0f, rotationYDegrees, 0.0f, coffeePosition);
	SetShaderColor(0.24f, 0.14f, 0.08f, 1.0f); // dark brewed coffee
	SetShaderMaterial("leather2"); // reuses the low-sheen response; liquid doesn't need ceramic gloss
	m_basicMeshes->DrawCylinderMesh();

	// MUG HANDLE - torus, rotated 90 degrees about Z so the ring
	// stands on edge (hole facing outward) instead of lying flat,
	// then offset away from the body along whichever direction the
	// mug is currently facing so it stays attached as the mug rotate
	float handleDistance = 0.60f * scaleFactor;
	float offsetX = cos(glm::radians(rotationYDegrees)) * handleDistance;
	float offsetZ = -sin(glm::radians(rotationYDegrees)) * handleDistance;
	handlePosition = positionXYZ + glm::vec3(offsetX, 0.40f * scaleFactor, offsetZ);

	scaleXYZ = glm::vec3(0.24f, 0.24f, 0.16f) * scaleFactor;
	SetTransformations(scaleXYZ, 0.0f, rotationYDegrees, 90.0f, handlePosition);
	SetShaderTexture("ceramic");
	SetShaderMaterial("ceramic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes.
 *
 *  Draws the complete six book stack scene based on the
 *  2D reference image. A light grey plane acts as the shelf
 *  surface, and six box primitives are stacked along the Y
 *  axis, each with individually tuned scale, rotation, and
 *  translation values to match the reference photo.
 *  Books 4, 5, and 6 use three boxes each (top cover, pages,
 *  bottom cover) to simulate realistic book construction.
 *
 *  Stack layout from the bottom to top:
 *  BOOK 1: Thin grey hardcover      - widest, thinnest book in the stack.
 *  BOOK 2: Thick all-white book     - tallest spine, bright white cover.
 *  BOOK 3: Light cream paperback    - thin, slightly narrower than book 2.
 *  BOOK 4: Black cover book         - three-part construction, cream pages.
 *  BOOK 5: Off-white cover book     - three-part construction, cream pages.
 *  BOOK 6: Dark grey notebook       - three-part construction, narrowest on top.
 * 
 * Transformation order: scale, rotate, translate.
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/******************************************************************/
	// DESK SURFACE - flat white plane at origin
	
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wood");
	SetShaderMaterial("wood");
	SetTextureUVScale(10.0f, 5.0f);
	m_basicMeshes->DrawPlaneMesh();
	/******************************************************************/

	/******************************************************************/
	// BOOK 1 - Top cover - dark grey hardcover
	
	scaleXYZ = glm::vec3(3.0f, 0.03f, 2.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.2f, 0.135f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("fabric");
	SetShaderMaterial("fabric");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 1 - lite grey pages
	
	scaleXYZ = glm::vec3(3.0f, 0.09f, 2.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.2f, 0.08f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("fabric2");
	SetShaderMaterial("fabric2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 1 - Bottom cover - dark grey hardcover
	
	scaleXYZ = glm::vec3(3.0f, 0.03f, 2.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.2f, 0.02f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("fabric");
	SetShaderMaterial("fabric");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 1 - Left side cover - dark grey hardcover
	
	scaleXYZ = glm::vec3(0.02f, 0.145f, 2.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.66f, 0.0775f, -0.31f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("fabric");
	SetShaderMaterial("fabric");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// BOOK 2 - Top cover - white
	
	scaleXYZ = glm::vec3(2.85f, 0.04f, 1.98f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.1f, 0.63f, 0.1f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("paper");
	SetShaderMaterial("paper");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 2 - Thick book/all white pages - bottom, widest
	
	scaleXYZ = glm::vec3(2.80f, 0.44f, 1.85f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.1f, 0.4f, 0.1f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("white");
	SetShaderMaterial("white");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 2 - Bottom cover - white
	
	scaleXYZ = glm::vec3(2.85f, 0.04f, 1.98f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.1f, 0.165f, 0.1f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("paper");
	SetShaderMaterial("paper");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 2 - Left side cover - white
	
	scaleXYZ = glm::vec3(0.04f, 0.505f, 1.984f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.48f, 0.3975f, -0.196f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("paper");
	SetShaderMaterial("paper");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// BOOK 3 - Top cover - thin
	
	scaleXYZ = glm::vec3(2.95f, 0.03f, 2.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -17.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.1f, 0.805f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("rough");
	SetShaderMaterial("rough");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// BOOK 3 - All white paperback book - thin

	scaleXYZ = glm::vec3(2.9f, 0.11f, 2.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -17.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.1f, 0.735f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leather2");
	SetShaderMaterial("leather2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// BOOK 3 - Bottom cover - thin

	scaleXYZ = glm::vec3(2.95f, 0.03f, 2.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -17.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.1f, 0.665f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("rough");
	SetShaderMaterial("rough");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// BOOK 3 - Left side cover - thin

	scaleXYZ = glm::vec3(0.04f, 0.17f, 2.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -17.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.49f, 0.735f, -0.44f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("rough");
	SetShaderMaterial("rough");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// BOOK 4 - Top cover - black, thin 
    
	scaleXYZ = glm::vec3(2.4f, 0.03f, 1.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.095f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leather");
	SetShaderMaterial("leather");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 4 - White pages - middle
	
	scaleXYZ = glm::vec3(2.35f, 0.22f, 1.60f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.96f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pages");
	SetShaderMaterial("pages");
	SetTextureUVScale(1.0f, 6.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 4 - Bottom cover - near-black, thin
	
	scaleXYZ = glm::vec3(2.4f, 0.03f, 1.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.825f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leather");
	SetShaderMaterial("leather");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
    // BOOK 4 - Black strap/band across front
   
	scaleXYZ = glm::vec3(0.10f, 0.28f, 0.02f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.6f, 0.96f, 0.95f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leather");
	SetShaderMaterial("leather");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 4 - Left side cover
	
	scaleXYZ = glm::vec3(0.1f, 0.30f, 1.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -12.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.21f, 0.96f, -0.27f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leather");
	SetShaderMaterial("leather");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// BOOK 5 - Top cover - off white, thin
	
	scaleXYZ = glm::vec3(2.2f, 0.03f, 1.57f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -14.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.275f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.89f, 0.89f, 0.87f, 1.0f); // light grey
	SetShaderMaterial("paper");
	m_basicMeshes->DrawBoxMesh();

	
    // BOOK 5 - Light cream pages - middle
    
	scaleXYZ = glm::vec3(2.15f, 0.14f, 1.55f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -14.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.02f, 1.2f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("carpet");
	SetShaderMaterial("carpet");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 5 - Bottom cover off-white, thin
	
	scaleXYZ = glm::vec3(2.2f, 0.03f, 1.57f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -14.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.125f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.89f, 0.89f, 0.87f, 1.0f); // off-white cover
	SetShaderMaterial("paper");
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 5 - Left side of the cover
	
	scaleXYZ = glm::vec3(0.1f, 0.18f, 1.57f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -14.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.11f, 1.2f, -0.28f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.89f, 0.89f, 0.87f, 1.0f); // off-white cover
	SetShaderMaterial("paper");
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// BOOK 6 - Cover for Small dark notebook - Book on top, narrowest
	
	scaleXYZ = glm::vec3(2.0f, 0.03f, 1.55f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.445f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 6 - White pages - slightly smaller than cover
	
	scaleXYZ = glm::vec3(1.95f, 0.12f, 1.35f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.0f, 1.375f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("fabric2");
	SetShaderMaterial("fabric2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	
	// BOOK 6 - Cover for Small dark notebook - top, narrowest
	
	scaleXYZ = glm::vec3(2.0f, 0.03f, 1.55f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.305f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// BOOK 6 - Left side cover
	
	scaleXYZ = glm::vec3(0.1f, 0.17f, 1.55f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.96f, 1.375f, -0.26f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// COFFEE MUG - reusable multi-primitive object (cylinder body +
	// torus handle), placed to the right of the book stack with the
	// handle facing back toward it
	RenderMug(glm::vec3(4.6f, 0.01f, -1.1f), -20.0f, 1.0f);
	/******************************************************************/

	/******************************************************************/
	// STONE PAPERWEIGHT - small sphere resting on the desk near the mug

	scaleXYZ = glm::vec3(0.50f, 0.50f, 0.50f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.6f, 0.50f, -0.90f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/
}

