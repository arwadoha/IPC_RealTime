#include <GL/glut.h>

float personX = 0.0f;
float personY = 0.0f;
float personAngle = 0.0f;

void drawPerson() {
    // Head
    glPushMatrix();
    glTranslatef(personX, personY + 1.0f, 0.0f);
    glutSolidSphere(0.1, 20, 20);
    glPopMatrix();

    // Body
    glPushMatrix();
    glTranslatef(personX, personY + 0.8f, 0.0f);
    glScalef(0.2, 0.3, 0.1);
    glutSolidCube(1.0);
    glPopMatrix();

    // Left Leg
    glPushMatrix();
    glTranslatef(personX - 0.05f, personY + 0.4f, 0.0f);
    glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
    glScalef(0.1, 0.5, 0.1);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right Leg
    glPushMatrix();
    glTranslatef(personX + 0.05f, personY + 0.4f, 0.0f);
    glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
    glScalef(0.1, 0.5, 0.1);
    glutSolidCube(1.0);
    glPopMatrix();

    // Left Arm
    glPushMatrix();
    glTranslatef(personX - 0.1f, personY + 0.8f, 0.0f);
    glRotatef(personAngle, 0.0f, 0.0f, 1.0f);
    glScalef(0.1, 0.3, 0.1);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right Arm
    glPushMatrix();
    glTranslatef(personX + 0.1f, personY + 0.8f, 0.0f);
    glRotatef(-personAngle, 0.0f, 0.0f, 1.0f);
    glScalef(0.1, 0.3, 0.1);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawBackground() {
    // Floor
    glColor3f(0.9f, 0.9f, 0.9f);  // Light gray color for the floor
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);
    glScalef(10.0, 0.1, 5.0);
    glutSolidCube(1.0);
    glPopMatrix();

    // Aisles
    glColor3f(1.0f, 0.0f, 0.0f);  // Gray color for aisles
    for (float x = -2.5; x <= 2.5; x += 2.5) {
        glPushMatrix();
        glTranslatef(x, 0.0f, 0.0f);
        glScalef(0.2, 1.0, 4.0);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // Shelves
    glColor3f(0.0f, 1.0f, 0.0f);  // Dark gray color for shelves
    for (float z = -1.8; z <= 1.8; z += 0.6) {
        glPushMatrix();
        glTranslatef(0.0f, 0.5f, z);
        glScalef(8.0, 0.1, 0.4);
        glutSolidCube(1.0);
        glPopMatrix();

        // Items on Shelves (colored cubes)
        glColor3f(1.0f, 0.75f, 0.8f);  // Pink color
        glPushMatrix();
        glTranslatef(-1.0f, 0.7f, z);
        glutSolidCube(0.1);
        glPopMatrix();

        glColor3f(0.0f, 1.0f, 0.0f);  // White color
        glPushMatrix();
        glTranslatef(0.0f, 0.7f, z);
        glutSolidCube(0.1);
        glPopMatrix();

        glColor3f(1.0f, 0.0f, 0.0f);  // Yellow color
        glPushMatrix();
        glTranslatef(1.0f, 0.7f, z);
        glutSolidCube(0.1);
        glPopMatrix();
        
        glColor3f(1.0f, 1.0f, 1.0f);  // Yellow color
        glPushMatrix();
        glTranslatef(1.0f, 0.7f, z);
        glutSolidCube(0.1);
        glPopMatrix();
        
    }

    // Cashier Counter
     glColor3f(1.0f, 0.75f, 0.8f);  // Pink color
    glPushMatrix();
    glTranslatef(2.8f, 0.0f, 0.0f);
    glScalef(0.2, 1.0, 4.0);
    glutSolidCube(1.0);
    glPopMatrix();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw background
    drawBackground();

    // Draw the walking person
    drawPerson();

    glFlush();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)w / (GLfloat)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0f, 2.0f, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_RIGHT:
            personX += 0.1f;
            personAngle -= 10.0f;
            break;
        case GLUT_KEY_LEFT:
            personX -= 0.1f;
            personAngle += 10.0f;
            break;
        case GLUT_KEY_UP:
            personY += 0.1f;
            break;
        case GLUT_KEY_DOWN:
            personY -= 0.1f;
            break;
    }

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("Shopping in the Supermarket");
    glutInitWindowSize(800, 600);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);

    glEnable(GL_DEPTH_TEST);

    glutMainLoop();

    return 0;
}

