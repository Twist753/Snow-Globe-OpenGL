/*
    !!  Controls:  !!
    Shake -> S
    Night Mode -> N
    Camera Angle Reset -> R
    Zoom in/out -> +/-
    Rotate View -> Left Click
    Rotate Globe -> Right Click   :)
*/

#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <random>
#include <ctime>

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Snow parameters
const int NUM_SNOWFLAKES = 800;
const float GLOBE_RADIUS = 5.0f;
const float BASE_HEIGHT = 1.2f;

// Camera variables
float cameraDistance = 10.0f;
float cameraAngleX = 15.0f;
float cameraAngleY = 30.0f;

// Mouse control variables
int lastMouseX = 0;
int lastMouseY = 0;
bool mouseLeftDown = false;
bool mouseRightDown = false;
float rotationSpeed = 0.0f;
float globeRotationY = 0.0f;
bool isRotating = false;

// Shaking variables
bool isShaking = false;
float shakeMagnitude = 0.0f;
float shakeDecay = 0.95f;
float maxShakeMagnitude = 1.0f;

// Night mode variables
bool isNightMode = false;
float dayNightTransition = 0.0f; // 0.0 = day, 1.0 = night
float transitionSpeed = 0.02f; // Speed of day/night transition

// Star variables for night sky
const int NUM_STARS = 200;
struct Star {
    float x, y, z;
    float brightness;
    float twinkleRate;
    float twinkleOffset;
};
std::vector<Star> stars;

// Time tracking
int lastTime = 0;
float deltaTime = 0.0f;
float totalTime = 0.0f; // Total elapsed time for animations

// Snowflake struct
struct Snowflake {
    float x, y, z;
    float size;
    float speed;
    float angle; // Angle for particle rotation
    float vx, vy, vz; // Velocity components
    float sparkleRate; // Rate at which snowflake sparkles in night mode
    float sparklePhase; // Phase offset for sparkling effect
};

std::vector<Snowflake> snowflakes;

// Hut light struct
struct HutLight {
    float x, y, z; // Position
    float r, g, b; // Color
    float blinkRate; // How fast the light blinks (if it does)
    float blinkPhase; // Phase offset for blinking
    bool blinks; // Whether this light blinks or stays steady
};

std::vector<HutLight> hutLights;

// Initialize stars for night sky
void initStars() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-20.0f, 20.0f);
    std::uniform_real_distribution<float> heightDist(5.0f, 20.0f);
    std::uniform_real_distribution<float> brightDist(0.5f, 1.0f);
    std::uniform_real_distribution<float> rateDist(0.5f, 3.0f);
    std::uniform_real_distribution<float> offsetDist(0.0f, 2.0f * M_PI);

    stars.clear();
    for (int i = 0; i < NUM_STARS; ++i) {
        Star star;
        star.x = posDist(gen);
        star.y = heightDist(gen);
        star.z = posDist(gen);
        star.brightness = brightDist(gen);
        star.twinkleRate = rateDist(gen);
        star.twinkleOffset = offsetDist(gen);
        stars.push_back(star);
    }
}

// Initialize hut lights
void initHutLights() {
    hutLights.clear();

    // Lights around the door
    HutLight doorLight1 = { 0.2f, -0.1f, 0.55f, 1.0f, 0.8f, 0.0f, 1.5f, 0.0f, false }; // Steady yellow
    HutLight doorLight2 = { -0.2f, -0.1f, 0.55f, 1.0f, 0.8f, 0.0f, 1.5f, 0.0f, false }; // Steady yellow
    hutLights.push_back(doorLight1);
    hutLights.push_back(doorLight2);

    // Window lights
    HutLight windowLight1 = { 0.4f, 0.1f, 0.55f, 0.9f, 0.9f, 0.7f, 0.0f, 0.0f, false }; // Steady warm white
    HutLight windowLight2 = { -0.4f, 0.1f, 0.55f, 0.9f, 0.9f, 0.7f, 0.0f, 0.0f, false }; // Steady warm white
    hutLights.push_back(windowLight1);
    hutLights.push_back(windowLight2);

    // Roof decoration lights
    for (int i = 0; i < 8; i++) {
        float angle = i * (2.0f * M_PI / 8.0f);
        HutLight roofLight;
        roofLight.x = 0.7f * cos(angle);
        roofLight.y = 0.3f;
        roofLight.z = 0.7f * sin(angle);

        // Alternating colors
        if (i % 3 == 0) {
            roofLight.r = 1.0f;
            roofLight.g = 0.0f;
            roofLight.b = 0.0f; // Red
        }
        else if (i % 3 == 1) {
            roofLight.r = 0.0f;
            roofLight.g = 1.0f;
            roofLight.b = 0.0f; // Green
        }
        else {
            roofLight.r = 0.0f;
            roofLight.g = 0.5f;
            roofLight.b = 1.0f; // Blue
        }

        roofLight.blinkRate = 0.5f + (i * 0.2f); // Different blink rates
        roofLight.blinkPhase = i * 0.7f; // Different phases
        roofLight.blinks = true; // These lights blink

        hutLights.push_back(roofLight);
    }

    // Chimney light 
    HutLight chimneyLight = { 0.3f, 1.1f, 0.0f, 1.0f, 0.6f, 0.2f, 2.0f, 0.0f, true }; // Blinking orange
    hutLights.push_back(chimneyLight);
}

// Initialize snowflakes randomly within the globe
void initSnowflakes() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> radiusDist(0.0f, GLOBE_RADIUS * 0.9f);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> heightDist(-GLOBE_RADIUS * 0.8f, GLOBE_RADIUS * 0.9f);
    std::uniform_real_distribution<float> sizeDist(0.02f, 0.08f);
    std::uniform_real_distribution<float> speedDist(0.3f, 1.0f);
    std::uniform_real_distribution<float> velDist(-0.01f, 0.01f);
    std::uniform_real_distribution<float> sparkleRateDist(2.0f, 6.0f);
    std::uniform_real_distribution<float> sparkleOffsetDist(0.0f, 2.0f * M_PI);

    snowflakes.clear();
    for (int i = 0; i < NUM_SNOWFLAKES; ++i) {
        float radius = radiusDist(gen);
        float angle = angleDist(gen);
        float height = heightDist(gen);

        Snowflake flake;
        flake.x = radius * cos(angle);
        flake.y = height;
        flake.z = radius * sin(angle);
        flake.size = sizeDist(gen);
        flake.speed = speedDist(gen);
        flake.angle = angleDist(gen);
        flake.vx = velDist(gen);
        flake.vy = -flake.speed * 0.01f; // Initial downward velocity
        flake.vz = velDist(gen);
        flake.sparkleRate = sparkleRateDist(gen);
        flake.sparklePhase = sparkleOffsetDist(gen);

        snowflakes.push_back(flake);
    }
}

// Initialize OpenGL settings
void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set up light
    GLfloat lightPosition[] = { 10.0f, 10.0f, 10.0f, 1.0f };
    GLfloat lightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    // Set up material properties
    GLfloat materialSpecular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat materialShininess[] = { 50.0f };

    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);

    // Set day background
    glClearColor(0.5f, 0.8f, 0.98f, 1.0f);

    initSnowflakes();
    initStars();
    initHutLights();

    lastTime = glutGet(GLUT_ELAPSED_TIME);
}

// Draw the snow globe base
void drawBase() {
    glPushMatrix();

    // Move base below the globe
    glTranslatef(0.0f, -GLOBE_RADIUS, 0.0f);

    // Rotate base around X-axis (adjust angle as needed)
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);  // Rotate 90 degrees around X-axis

    // Wood-like color
    glColor3f(0.3f, 0.2f, 0.1f);

    // Draw cylinder
    GLUquadricObj* base = gluNewQuadric();
    gluQuadricDrawStyle(base, GLU_FILL);
    gluQuadricNormals(base, GLU_SMOOTH);
    gluCylinder(base, GLOBE_RADIUS * 0.9f, GLOBE_RADIUS * 0.9f, BASE_HEIGHT, 32, 1);

    // Top disk
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, BASE_HEIGHT);  // Since we've rotated, z is "up"
    GLUquadricObj* topDisk = gluNewQuadric();
    gluQuadricDrawStyle(topDisk, GLU_FILL);
    gluQuadricNormals(topDisk, GLU_SMOOTH);
    gluDisk(topDisk, 0.0f, GLOBE_RADIUS * 0.9f, 32, 1);
    gluDeleteQuadric(topDisk);
    glPopMatrix();

    // Bottom disk (optional)
    GLUquadricObj* bottomDisk = gluNewQuadric();
    gluQuadricDrawStyle(bottomDisk, GLU_FILL);
    gluQuadricNormals(bottomDisk, GLU_SMOOTH);
    gluDisk(bottomDisk, 0.0f, GLOBE_RADIUS * 0.9f, 32, 1);
    gluDeleteQuadric(bottomDisk);

    // Cleanup
    gluDeleteQuadric(base);
    glPopMatrix();
}

// Draw the glass globe
void drawGlobe() {
    glPushMatrix();
    // Draw transparent glass globe
    glColor4f(0.8f, 0.8f, 0.9f, 0.3f);
    glutSolidSphere(GLOBE_RADIUS, 50, 50);
    glPopMatrix();
}

// Draw snow inside the globe with optional sparkle effect
void drawSnow() {
    glDisable(GL_LIGHTING);

    // Draw active snowflakes
    for (const auto& flake : snowflakes) {
        glPushMatrix();
        glTranslatef(flake.x, flake.y, flake.z);
        glRotatef(flake.angle, 0.0f, 1.0f, 0.0f);

        // Determine snowflake color based on night mode
        if (isNightMode) {
            // In night mode, add sparkle effect
            float sparkle = sin(totalTime * flake.sparkleRate + flake.sparklePhase);
            sparkle = (sparkle + 1.0f) * 0.5f; // Convert to [0,1] range

            // Create a sparkling effect with slight color variation
            float brightness = 0.5f + 0.5f * sparkle;
            float blueHint = 0.6f + 0.4f * sparkle;
            glColor3f(brightness, brightness, blueHint);
        }
        else {
            // Normal white snow in day mode with slight transition
            float brightness = 1.0f - (dayNightTransition * 0.3f);
            glColor3f(brightness, brightness, brightness);
        }

        // Draw a small quad for each snowflake
        glBegin(GL_QUADS);
        glVertex3f(-flake.size, -flake.size, 0.0f);
        glVertex3f(flake.size, -flake.size, 0.0f);
        glVertex3f(flake.size, flake.size, 0.0f);
        glVertex3f(-flake.size, flake.size, 0.0f);
        glEnd();

        // Draw a perpendicular quad for 3D effect
        glBegin(GL_QUADS);
        glVertex3f(0.0f, -flake.size, -flake.size);
        glVertex3f(0.0f, flake.size, -flake.size);
        glVertex3f(0.0f, flake.size, flake.size);
        glVertex3f(0.0f, -flake.size, flake.size);
        glEnd();
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
}

// Draw stars in night mode
void drawStars() {
    if (dayNightTransition <= 0.0f) return; // Don't draw stars in day mode

    glDisable(GL_LIGHTING);

    for (const auto& star : stars) {
        // Calculate star brightness with twinkling effect
        float twinkle = sin(totalTime * star.twinkleRate + star.twinkleOffset);
        twinkle = (twinkle + 1.0f) * 0.5f; // Convert to [0,1] range
        float brightness = star.brightness * (0.7f + 0.3f * twinkle) * dayNightTransition;

        glColor3f(brightness, brightness, brightness);

        glPushMatrix();
        glTranslatef(star.x, star.y, star.z);

        // Simple point for stars
        glPointSize(1.5f);
        glBegin(GL_POINTS);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
}

// Draw decorative lights on the hut
void drawHutLights(float x, float y, float z) {
    if (dayNightTransition <= 0.1f) return; // Only visible at night

    // Enable lighting for glow effects
    glEnable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST); // Draw lights on top

    for (const auto& light : hutLights) {
        float intensity = 1.0f;

        // Apply blinking effect if this light blinks
        if (light.blinks) {
            float blink = sin(totalTime * light.blinkRate + light.blinkPhase);
            intensity = (blink + 1.0f) * 0.5f; // Convert to [0,1] range
            intensity = 0.4f + (0.6f * intensity); // Keep a minimum brightness
        }

        // Scale intensity by day/night transition
        intensity *= dayNightTransition;

        // Set light color with current intensity
        GLfloat emission[] = { light.r * intensity, light.g * intensity, light.b * intensity, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, emission);

        glPushMatrix();
        // Position relative to input position (which is hut's position)
        glTranslatef(x + light.x, y + light.y, z + light.z);

        // Draw a small sphere for the light
        glutSolidSphere(0.05f, 8, 8);

        // Optional: Draw a larger, dimmer sphere for glow effect
        glColor4f(light.r, light.g, light.b, 0.2f * intensity);
        glutSolidSphere(0.12f, 8, 8);
        glPopMatrix();
    }

    // Reset emission
    GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);

    glEnable(GL_DEPTH_TEST);
}

// Draw the hut inside the globe
void drawHut() {
    glPushMatrix();
    glRotatef(globeRotationY, 0.0f, 1.0f, 0.0f);

    // Ground/snow layer - Adjusted for night mode
    if (isNightMode) {
        // Bluish snow at night
        glColor3f(0.7f - (0.2f * dayNightTransition),
            0.7f - (0.0f * dayNightTransition),
            0.8f + (0.1f * dayNightTransition));
    }
    else {
        // White snow in day
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    glPushMatrix();
    glTranslatef(0.0f, -GLOBE_RADIUS + 0.5f, 0.0f);
    glScalef(1.0f, 0.15f, 1.0f);
    glutSolidSphere(GLOBE_RADIUS * 0.8f, 30, 30);
    glPopMatrix();

    // Draw the hut
    glPushMatrix();
    float hutX = 0.0f;
    float hutY = -GLOBE_RADIUS + 1.2f;
    float hutZ = 0.0f;
    glTranslatef(hutX, hutY, hutZ);

    // Hut body - adjust color based on day/night
    float woodDarkening = 0.2f * dayNightTransition; // Darken wood at night
    glColor3f(0.6f - woodDarkening, 0.4f - woodDarkening, 0.2f - woodDarkening);
    glPushMatrix();
    glScalef(1.2f, 1.0f, 1.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Roof - adjust color based on day/night
    glColor3f(0.3f - (0.1f * dayNightTransition),
        0.1f - (0.05f * dayNightTransition),
        0.1f - (0.05f * dayNightTransition)); // Darker red roof at night
    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(1.0f, 0.8f, 12, 12);
    glPopMatrix();

    // Door with glow effect at night
    if (isNightMode) {
        // Door border (darker)
        glColor3f(0.2f - (0.1f * dayNightTransition),
            0.1f - (0.05f * dayNightTransition),
            0.05f - (0.03f * dayNightTransition));
    }
    else {
        // Normal door
        glColor3f(0.3f, 0.2f, 0.1f);
    }

    glPushMatrix();
    glTranslatef(0.0f, -0.25f, 0.51f);
    glScalef(0.4f, 0.5f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Windows with glow at night
    if (isNightMode) {
        // Glowing warm light from windows at night
        glColor3f(0.9f * dayNightTransition,
            0.8f * dayNightTransition,
            0.2f * dayNightTransition);

        // Optional: Add emission for stronger glow
        GLfloat windowEmission[] = { 0.5f * dayNightTransition, 0.4f * dayNightTransition, 0.1f * dayNightTransition, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, windowEmission);
    }
    else {
        // Normal windows during day
        glColor3f(0.9f, 0.9f, 1.0f);
        GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
    }

    // Left window
    glPushMatrix();
    glTranslatef(-0.4f, 0.1f, 0.51f);
    glScalef(0.3f, 0.3f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Right window
    glPushMatrix();
    glTranslatef(0.4f, 0.1f, 0.51f);
    glScalef(0.3f, 0.3f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Reset emission
    GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);

    // Chimney with smoke
    glColor3f(0.2f - (0.1f * dayNightTransition),
        0.2f - (0.1f * dayNightTransition),
        0.2f - (0.1f * dayNightTransition)); // Darker at night
    glPushMatrix();
    glTranslatef(0.3f, 0.9f, 0.0f);
    glScalef(0.2f, 0.5f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Add chimney smoke (only visible in day mode)
    if (!isNightMode) {
        glDisable(GL_LIGHTING);
        glColor4f(0.8f, 0.8f, 0.8f, 0.5f - (0.5f * dayNightTransition));

        for (int i = 0; i < 5; i++) {
            float height = 0.2f + (i * 0.1f);
            float wobble = sin(totalTime * 1.5f + i) * 0.05f;

            glPushMatrix();
            glTranslatef(0.3f + wobble, 1.2f + height, 0.0f);
            glScalef(0.15f + (height * 0.1f), 0.1f, 0.15f + (height * 0.1f));
            glutSolidSphere(1.0f, 8, 8);
            glPopMatrix();
        }

        glEnable(GL_LIGHTING);
    }

    // Add hut lights in night mode
    drawHutLights(0.0f, 0.0f, 0.0f);

    glPopMatrix();
    glPopMatrix();
}

// Function to rotate a point around the y-axis
void rotatePointY(float& x, float& z, float angle) {
    float radAngle = angle * M_PI / 180.0f;
    float newX = x * cos(radAngle) - z * sin(radAngle);
    float newZ = x * sin(radAngle) + z * cos(radAngle);
    x = newX;
    z = newZ;
}

// Update background color based on day/night transition
void updateBackgroundColor() {
    if (isNightMode) {
        // Transition to night (dark blue)
        if (dayNightTransition < 1.0f) {
            dayNightTransition += transitionSpeed;
            if (dayNightTransition > 1.0f) dayNightTransition = 1.0f;
        }
    }
    else {
        // Transition to day (light blue)
        if (dayNightTransition > 0.0f) {
            dayNightTransition -= transitionSpeed;
            if (dayNightTransition < 0.0f) dayNightTransition = 0.0f;
        }
    }

    // Calculate background color based on transition value
    float r = 0.5f - (0.45f * dayNightTransition); // 0.5 (day) to 0.05 (night)
    float g = 0.8f - (0.75f * dayNightTransition); // 0.8 (day) to 0.05 (night)
    float b = 0.98f - (0.8f * dayNightTransition); // 0.98 (day) to 0.18 (night)

    glClearColor(r, g, b, 1.0f);
}

// Update snow positions and handle shaking and rotation effects
void updateSnow() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (currentTime - lastTime) / 1000.0f; // Convert to seconds
    lastTime = currentTime;
    totalTime += deltaTime;

    // Update background based on day/night mode
    updateBackgroundColor();

    // Apply shake decay
    if (isShaking) {
        shakeMagnitude *= shakeDecay;
        if (shakeMagnitude < 0.01f) {
            isShaking = false;
            shakeMagnitude = 0.0f;
        }
    }

    // Calculate rotation effect on particles
    float rotationEffect = 0.0f;
    if (isRotating) {
        rotationEffect = rotationSpeed * 2.0f;
        rotationSpeed *= 0.98f;  // Damping

        if (fabs(rotationSpeed) < 0.05f) {
            isRotating = false;
            rotationSpeed = 0.0f;
        }
    }

    // Update globe rotation
    if (isRotating) {
        globeRotationY += rotationSpeed;
        // Keep angle between 0-360
        if (globeRotationY > 360.0f) globeRotationY -= 360.0f;
        if (globeRotationY < 0.0f) globeRotationY += 360.0f;
    }

    // Random generator for turbulence
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> turbDist(-0.01f, 0.01f);

    // Update falling snowflakes 
    for (auto& flake : snowflakes) {
        // Apply gravity
        flake.vy -= 0.0005f * flake.speed;

        // Apply random turbulence (subtle air movement)
        if (!isShaking) {
            flake.vx += turbDist(gen) * 0.01f;
            flake.vz += turbDist(gen) * 0.01f;
        }

        // Apply shaking effect
        if (isShaking) {
            std::uniform_real_distribution<float> shakeDist(-1.0f, 1.0f);

            flake.vx += shakeMagnitude * shakeDist(gen) * 0.05f;
            flake.vy += shakeMagnitude * shakeDist(gen) * 0.05f;
            flake.vz += shakeMagnitude * shakeDist(gen) * 0.05f;

            // Spin the snowflakes faster during shaking
            flake.angle += shakeMagnitude * 10.0f;
        }
        else {
            // Apply rotation effect
            if (isRotating) {
                // Apply opposite force to simulate inertia
                float forceZ = rotationEffect * flake.x * 0.01f;
                float forceX = -rotationEffect * flake.z * 0.01f;

                flake.vx += forceX;
                flake.vz += forceZ;

                flake.angle += rotationEffect * 0.5f;
            }
            else {
                // Gentle rotation even when not shaking or rotating
                flake.angle += 0.2f * flake.speed;
            }
        }

        // Apply velocity damping (air resistance)
        flake.vx *= 0.99f;
        flake.vy *= 0.99f;
        flake.vz *= 0.99f;

        // Update positions
        flake.x += flake.vx * deltaTime * 60.0f;
        flake.y += flake.vy * deltaTime * 60.0f;
        flake.z += flake.vz * deltaTime * 60.0f;

        // Check if snowflake hits the ground
        float minY = -GLOBE_RADIUS + 0.5f;
        if (flake.y < minY) {
            flake.y = minY;
            flake.vy = -flake.vy * 0.3f; // Bounce with energy loss

            // Some chance of the snowflake getting back up when shaking
            if (isShaking && shakeMagnitude > 0.5f) {
                std::uniform_real_distribution<float> heightDist(-GLOBE_RADIUS * 0.5f, GLOBE_RADIUS * 0.9f);
                std::uniform_real_distribution<float> chance(0.0f, 1.0f);

                if (chance(gen) < shakeMagnitude * 0.2f) {
                    flake.y = heightDist(gen);
                    // Reset velocity for particles that get picked back up
                    flake.vx = turbDist(gen) * 0.05f;
                    flake.vy = turbDist(gen) * 0.02f;
                    flake.vz = turbDist(gen) * 0.05f;
                }
            }
        }

        // Check for collision with the hut
        float hutX = 0.0f;
        float hutZ = 0.0f;
        float hutSize = 0.8f;

        // Rotate hut position according to globe rotation
        float hutXRot = hutX;
        float hutZRot = hutZ;
        rotatePointY(hutXRot, hutZRot, globeRotationY);

        // Simple box collision with the hut
        if (flake.y < -GLOBE_RADIUS + 2.0f && flake.y > -GLOBE_RADIUS + 0.5f) {
            float dx = fabs(flake.x - hutXRot);
            float dz = fabs(flake.z - hutZRot);

            if (dx < hutSize && dz < hutSize) {
                // Collision response
                if (dx > dz) {
                    flake.x = hutXRot + (flake.x > hutXRot ? 1 : -1) * hutSize;
                    flake.vx = -flake.vx * 0.5f;
                }
                else {
                    flake.z = hutZRot + (flake.z > hutZRot ? 1 : -1) * hutSize;
                    flake.vz = -flake.vz * 0.5f;
                }
            }
        }

        // Check for collision with the globe boundary
        float distFromCenter = sqrt(flake.x * flake.x + flake.y * flake.y + flake.z * flake.z);
        if (distFromCenter > GLOBE_RADIUS * 0.95f) {
            // Calculate normal vector for reflection
            float nx = flake.x / distFromCenter;
            float ny = flake.y / distFromCenter;
            float nz = flake.z / distFromCenter;

            // Reflect velocity vector
            float dotProduct = flake.vx * nx + flake.vy * ny + flake.vz * nz;
            flake.vx = flake.vx - 2 * dotProduct * nx;
            flake.vy = flake.vy - 2 * dotProduct * ny;
            flake.vz = flake.vz - 2 * dotProduct * nz;

            // Reduce energy (damping)
            flake.vx *= 0.8f;
            flake.vy *= 0.8f;
            flake.vz *= 0.8f;

            // Place slightly inside the boundary
            flake.x = nx * GLOBE_RADIUS * 0.94f;
            flake.y = ny * GLOBE_RADIUS * 0.94f;
            flake.z = nz * GLOBE_RADIUS * 0.94f;
        }
    }
}

// Function to render the scene
void display() {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the camera position
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Apply camera transformations with shake effect
    if (isShaking) {
        float shakeX = sin(totalTime * 20.0f) * shakeMagnitude * 0.1f;
        float shakeY = cos(totalTime * 15.0f) * shakeMagnitude * 0.1f;
        gluLookAt(
            0.0f, 0.0f, cameraDistance + shakeX,  // Eye position with shake
            0.0f, 0.0f, 0.0f,                    // Look at center
            0.0f, 1.0f + shakeY, 0.0f            // Up vector with shake
        );
    }
    else {
        gluLookAt(
            0.0f, 0.0f, cameraDistance,  // Eye position
            0.0f, 0.0f, 0.0f,           // Look at center
            0.0f, 1.0f, 0.0f            // Up vector
        );
    }

    // Apply camera rotations
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);

    // Adjust light position for night mode
    if (isNightMode) {
        GLfloat lightPos[] = { 10.0f, 10.0f, 10.0f, 1.0f };
        GLfloat nightAmbient[] = { 0.05f, 0.05f, 0.1f, 1.0f };
        GLfloat nightDiffuse[] = { 0.5f, 0.5f, 0.6f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT, nightAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, nightDiffuse);
    }
    else {
        GLfloat lightPos[] = { 10.0f, 10.0f, 10.0f, 1.0f };
        GLfloat dayAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        GLfloat dayDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT, dayAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, dayDiffuse);
    }

    // Draw stars in night mode
    drawStars();

    // Draw the base
    drawBase();

    // Draw the hut
    drawHut();

    // Draw snow
    drawSnow();

    // Draw the globe
    drawGlobe();

    // Swap buffers
    glutSwapBuffers();
}

// Function to update animations and physics
void update(int value) {
    // Update snow positions and physics
    updateSnow();

    // Request redisplay
    glutPostRedisplay();

    // Set up the next timer callback
    glutTimerFunc(16, update, 0); // ~60 FPS
}

// Function to handle window resizing
void reshape(int width, int height) {
    // Set the viewport to the full window
    glViewport(0, 0, width, height);

    // Set up the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);

    // Switch back to modelview matrix
    glMatrixMode(GL_MODELVIEW);
}

// Function to handle keyboard input
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 27: // ESC key
        exit(0);
        break;

    case 's': // Shake the globe
    case 'S':
        isShaking = true;
        shakeMagnitude = maxShakeMagnitude;
        break;

    case 'n': // Toggle night mode
    case 'N':
        isNightMode = !isNightMode;
        break;
    case '+': // Zoom in
    case '=':
        cameraDistance -= 0.5f;
        if (cameraDistance < 7.0f) cameraDistance = 7.0f;
        break;
    case '-': // Zoom out
    case '_':
        cameraDistance += 0.5f;
        if (cameraDistance > 20.0f) cameraDistance = 20.0f;
        break;
    case 'r': // Reset camera
    case 'R':
        cameraDistance = 10.0f;
        cameraAngleX = 15.0f;
        cameraAngleY = 30.0f;
        break;
    }

    glutPostRedisplay();
}

// Function to handle mouse movement
void mouseMotion(int x, int y) {
    if (mouseLeftDown) {
        // Camera rotation (view control)
        cameraAngleY += (x - lastMouseX) * 0.2f;
        cameraAngleX += (y - lastMouseY) * 0.2f;

        // Limit vertical angle to avoid flipping
        if (cameraAngleX > 89.0f) cameraAngleX = 89.0f;
        if (cameraAngleX < -89.0f) cameraAngleX = -89.0f;

        lastMouseX = x;
        lastMouseY = y;
    }
    else if (mouseRightDown) {
        // Globe rotation
        float currentRotation = (x - lastMouseX) * 0.5f;
        rotationSpeed = currentRotation;

        if (fabs(rotationSpeed) > 0.1f) {
            isRotating = true;
        }

        lastMouseX = x;
        lastMouseY = y;
    }
}

// Handle mouse clicks
void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseLeftDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else {
            mouseLeftDown = false;
        }
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseRightDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else {
            mouseRightDown = false;
        }
    }
}

// Function to handle special key presses
void specialKeyboard(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
        cameraAngleX += 5.0f;
        if (cameraAngleX > 89.0f) cameraAngleX = 89.0f;
        break;

    case GLUT_KEY_DOWN:
        cameraAngleX -= 5.0f;
        if (cameraAngleX < -89.0f) cameraAngleX = -89.0f;
        break;

    case GLUT_KEY_LEFT:
        cameraAngleY -= 5.0f;
        break;

    case GLUT_KEY_RIGHT:
        cameraAngleY += 5.0f;
        break;
    }

    glutPostRedisplay();
}

// Main function
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("3D Snow Globe");

    // Set up callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, update, 0);

    // Initialize OpenGL settings
    init();

    // Enter the main loop
    glutMainLoop();

    return 0;
}