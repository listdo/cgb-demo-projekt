/* STUNDENAUFWAND: 8h
* Tasten 1 bis 8 decken die Karten auf die Reihe beginnt links unten läuft dann bis rechts durch (als Karte 5 ist die obere linke Karte)
* Altnernative können die Karte auch mittels der linken Maustaste aufgedeckt werden.
* Ein Spieler kann pro Versuch zwei Karten aufdecken. Stimmen diese überein werden sie entfernt und der Spieler ist erneut an der Reihe.
* Sind es zwei untschiedliche Karte ist der zweite Spieler an der Reihe.
* Das Spiel endet wenn alle paar gefunden wurden oder durch drücken der ESC-Taste.
* Am Ende des Spiels werden die falschen Versuche der Spieler ausgegebn.
* (Da nicht anders gefordert befinden sich die Karten immer an der selben Stelle und die Paar untereinader)
*/

#include "GL\glew.h"
#include "GL\freeglut.h"
#include <iostream>

using namespace std;

// Create checkerboard texture0
#define checkImageWidth 64 //64
#define checkImageHeight 64 //64

static GLubyte checkImage[checkImageHeight][checkImageWidth][4];
static GLuint texNames[2];

// Data read from the header of the BMP file
unsigned char header[54]; // Each BMP file begins by a 54-bytes header
unsigned int dataPos;     // Position in the file where the actual data begins
unsigned int imageWidth, imageHeight;
unsigned int imageSize;   // = width*height*3
// Actual RGB data
unsigned char* imageData;


std::string str = "marbles.bmp"; // filename of bitmap-file 

const int NUMBER_OF_CARDS = 8;
const int NUMBER_OF_CORNERS_PER_CARD = 4;

char* filenameandpath = &str[0];  // hackaround for fopen_s 



// X,Y Coordinates for the corners of the cards
GLfloat cardVertexes[NUMBER_OF_CARDS * NUMBER_OF_CORNERS_PER_CARD][2] = {
	{-2.0f, -0.1f},{-2.0f, -0.75f},{-1.5f, -0.75f},{-1.5f, -0.1f},
	{-0.75f, -0.1f},{-0.75f, -0.75f},{-0.25f, -0.75f}, {-0.25f, -0.1f},
	{0.25f, -0.1f},{0.25f, -0.75f},{0.75f, -0.75f},{0.75f, -0.1f},
	{1.5f, -0.1f}, {1.5f, -0.75f} ,{2.0f, -0.75f}, {2.0f, -0.1f},
	{-2.0f, 0.75f},{-2.0f, 0.1f},{-1.5f, 0.1f},{-1.5f, 0.75f},
	{-0.75f, 0.75f},{-0.75f, 0.1f},{-0.25f, 0.1f},{-0.25f, 0.75f},
	{0.25f, 0.75f},{0.25f, 0.1f},{0.75f, 0.1f},{0.75f, 0.75f},
	{1.5f, 0.75f},{1.5f, 0.1f},{2.0f, 0.1f},{2.0f, 0.75f},
};

// Texels for the 'sub-texures' for the card fronts
GLfloat cardTexels[NUMBER_OF_CARDS][2] = {
	{0.0f, 0.0f}, {0.2f, 0.2f},
	{1.5f, 1.5f}, {1.7f, 1.7f},
	{0.4f, 0.4f}, {0.6f, 0.6f},
	{1.1f, 1.1f}, {1.3f, 1.3f}
};
bool are_cards_active[8] = { true,true,true,true,true,true,true,true }; // array to check which cards are still existing
int flipped_idx = -1;
int flipped_idx_2 = -1;


GLfloat move_away_z_offset = 0.0f;

int missed_count_p1, missed_count_p2 = 0;
int current_player = 0;
// GLUT Window ID
int windowid;


//loads the image from the given imagpath into imageData and sets imageSize, dataPas, imageWidth and imageHeight with the values from the header
int loadBMP_custom(const char* imagepath, unsigned char header[54], unsigned int* imageSize, unsigned int* dataPos, unsigned int* imageWidth, unsigned int* imageHeight, unsigned char** imageData) {
	FILE* file;
	fopen_s(&file, imagepath, "rb"); // Open the file
	if (!file) {
		cout << "Image could not be opened" << endl;
		return 0;
	}

	if (fread(header, 1, 54, file) != 54) { // If not 54 bytes read : problem
		cout << "Not a correct BMP file" << endl;
		return 0;
	}

	if (header[0] != 'B' || header[1] != 'M') {
		cout << "Not a correct BMP file" << endl;
		return 0;
	}

	// Read ints from the byte array
	*dataPos = *(int*)&(header[0x0A]);
	*imageSize = *(int*)&(header[0x22]);
	*imageWidth = *(int*)&(header[0x12]);
	*imageHeight = *(int*)&(header[0x16]);
	// Some BMP files are misformatted, guess missing information
	// 3 : one byte for each Red, Green and Blue component
	if (*imageSize == 0)    *imageSize = *imageWidth * *imageHeight * 3;
	if (*dataPos == 0)      *dataPos = 54; // The BMP header is done that way
	cout << "imageSize:" << *imageSize << endl;
	cout << "dataPos:" << *dataPos << endl;
	*imageData = new unsigned char[*imageSize]; // Create a buffer
	fread(*imageData, 1, *imageSize, file); // Read the actual data from the file into the buffer

	fclose(file); //Everything is in memory now, the file can be closed
}

GLfloat get_min_x_of_card(int idx) {
	return cardVertexes[idx * NUMBER_OF_CORNERS_PER_CARD + 0][0];
}

GLfloat get_max_x_of_card(int idx) {
	return cardVertexes[idx * NUMBER_OF_CORNERS_PER_CARD + 2][0];
}

GLfloat get_max_y_of_card(int idx) {
	return cardVertexes[idx * NUMBER_OF_CORNERS_PER_CARD + 0][1];
}

GLfloat get_min_y_of_card(int idx) {
	return cardVertexes[idx * NUMBER_OF_CORNERS_PER_CARD + 2][1];
}

//reads the image for the texture of the cards and initializes the texture
void initTextures(void) {
	loadBMP_custom(filenameandpath, header, &imageSize, &dataPos, &imageWidth, &imageHeight, &imageData);



	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, texNames);
	glBindTexture(GL_TEXTURE_2D, texNames[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // horizontal
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // vertical

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //wenn texel größer als pixel
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //wenn texel kleiner als pixel, linear sorgt für mehr unschärfe


	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, imageData);

}

//prints the failed tries per play and closes the window
void game_over() {
	cout << "--------------- " << "GAME OVER" << " ---------------" << endl;
	cout << "Player one failed tries: " << missed_count_p1 << endl;
	cout << "Player two failed tries: " << missed_count_p2 << endl;
	glutDestroyWindow(windowid);
	exit(0);
}

bool is_game_over() {
	for (int i = 0; i < NUMBER_OF_CARDS; i++)
	{
		if (are_cards_active[i])
			return false;
	}
	return true;
}

void flip_card(int value);

//handles the removing of the cards 
void move_away_cards(int) {
	if (move_away_z_offset >= -10) {
		move_away_z_offset -= 0.5;
		glutTimerFunc(80, move_away_cards, 0);
		glutPostRedisplay();
	}
	else {
		are_cards_active[flipped_idx] = false;
		are_cards_active[flipped_idx_2] = false;
		move_away_z_offset = 0;
		flip_card(2);
	}

}

//handles the games if a card is marked as flipped
void flip_card(int value) {
	if (value == 0) {
		glutPostRedisplay();
		if (flipped_idx_2 != -1)
			glutTimerFunc(200, flip_card, 1);
	}
	else {
		int next_player = current_player;
		int* p_missed_count;
		if (current_player == 0)
			p_missed_count = &missed_count_p1;
		else
			p_missed_count = &missed_count_p2;
		if (flipped_idx_2 == flipped_idx % (NUMBER_OF_CARDS/2)) {
			if (value < 2) {
				move_away_cards(0);
				return;
			}
			if (is_game_over()) {
				game_over();
			}

		}
		else {
			next_player = (current_player + 1) % 2;
			(*p_missed_count)++;
		}
		cout << "Current Player: " << current_player + 1 << "    " << "failed-tries: " << *p_missed_count << "    " << "Next Player: " << next_player + 1 << endl;
		current_player = next_player;
		flipped_idx_2 = -1;
		flipped_idx = -1;
		glutPostRedisplay();
	}
}

void draw_cards_backs(void) {
	for (int i = 0; i < 8; i++) {
		if (i != flipped_idx && i != flipped_idx_2 && are_cards_active[i]) { // don't draw backs of flippped cards or not longer existing onces
			glBegin(GL_QUADS);
				glTexCoord2f(0.5, 0.5); glVertex3f(cardVertexes[i * NUMBER_OF_CORNERS_PER_CARD + 0][0], cardVertexes[i * NUMBER_OF_CORNERS_PER_CARD + 0][1], 0.0f);
				glTexCoord2f(0.5, 2.0); glVertex3f(cardVertexes[i * NUMBER_OF_CORNERS_PER_CARD + 1][0], cardVertexes[i * NUMBER_OF_CORNERS_PER_CARD + 1][1], 0.0f);
				glTexCoord2f(2.0, 2.0); glVertex3f(cardVertexes[i * NUMBER_OF_CORNERS_PER_CARD + 2][0], cardVertexes[i * NUMBER_OF_CORNERS_PER_CARD + 2][1], 0.0f);
				glTexCoord2f(2.0, 0.5); glVertex3f(cardVertexes[i * NUMBER_OF_CORNERS_PER_CARD + 3][0], cardVertexes[i * NUMBER_OF_CORNERS_PER_CARD + 3][1], 0.0f);
			glEnd();
		}
	}
}

void draw_front_of_card(int idx) {
	if (idx < 0)
		return;
	glBegin(GL_QUADS);
		glTexCoord2f(cardTexels[2 * (idx % 4)][0], cardTexels[2 * (idx % 4)][1]); glVertex3f(cardVertexes[idx * 4][0], cardVertexes[idx * 4][1], move_away_z_offset);
		glTexCoord2f(cardTexels[2 * (idx % 4)][0], cardTexels[2 * (idx % 4) + 1][1]); glVertex3f(cardVertexes[idx * 4 + 1][0], cardVertexes[idx * 4 + 1][1], move_away_z_offset);
		glTexCoord2f(cardTexels[2 * (idx % 4) + 1][0], cardTexels[2 * (idx % 4) + 1][1]); glVertex3f(cardVertexes[idx * 4 + 2][0], cardVertexes[idx * 4 + 2][1], move_away_z_offset);
		glTexCoord2f(cardTexels[2 * (idx % 4) + 1][0], cardTexels[2 * (idx % 4)][1]); glVertex3f(cardVertexes[idx * 4 + 3][0], cardVertexes[idx * 4 + 3][1], move_away_z_offset);
	glEnd();
}

void draw_cards_front(void) {
	draw_front_of_card(flipped_idx);
	draw_front_of_card(flipped_idx_2);
}

//marks a card as flipped 
void set_flipped(int to_flip_idx) {
	if (!are_cards_active[to_flip_idx - 1]) {
		cout << "card is already removed" << endl;
		return;
	}
	if (flipped_idx != -1 && flipped_idx_2 != -1) {
		cout << "game not ready for input" << endl;
		return;
	}
	if (flipped_idx == to_flip_idx - 1) return;
	if (flipped_idx != -1)
		flipped_idx_2 = flipped_idx;

	flipped_idx = to_flip_idx - 1;
	flip_card(0);
	cout << "flipped card " << to_flip_idx << endl;
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); //GL_DECAL überschreibt farbe GL_MODULATE für fabmischung

	glBindTexture(GL_TEXTURE_2D, texNames[0]);
	glColor3f(0.5f, 0.0f, 0.0f);

	draw_cards_backs();

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); //GL_DECAL überschreibt farbe GL_MODULATE für fabmischung
	draw_cards_front();


	glFlush();
	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.0, 0.0, -4.5);
	glutSwapBuffers();
}

/*-[Keyboard Callback]-------------------------------------------------------*/
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case '1': 
		set_flipped(1);
		break;
	case '2': 
		set_flipped(2);
		break;
	case '3': 
		set_flipped(3);
		break;
	case '4':
		set_flipped(4);
		break;
	case '5': 
		set_flipped(5);
		break;
	case '6': 
		set_flipped(6);
		break;
	case '7': 
		set_flipped(7);
		break;
	case '8': 
		set_flipped(8);
		break;

	case 27: // Escape key
		game_over();
		break;
	}
	glutPostRedisplay();
}

/*-[MouseClick Callback]-----------------------------------------------------*/
void onMouseClick(int button, int state, int mouse_x, int mouse_y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		GLint viewport[4];
		GLdouble modelview[16];
		GLdouble projection[16];
		glGetIntegerv(GL_VIEWPORT, viewport);
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
		glGetDoublev(GL_PROJECTION_MATRIX, projection);

		GLfloat winx = (float)mouse_x;
		GLfloat winy = (float)viewport[3] - (float)mouse_y;
		GLfloat winz;

		glReadPixels(mouse_x, int(winy), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winz);
		GLdouble t_x, t_y, t_z;

		gluUnProject(winx, winy, winz, modelview, projection, viewport, &t_x, &t_y, &t_z);

		//check for all cards still existing of the mouse 'hits'
		for (int i = 0; i < 8; i++) {
			if (!are_cards_active[i])
				continue;
			GLfloat min_y, min_x, max_y, max_x;
			min_y = get_min_y_of_card(i);
			min_x = get_min_x_of_card(i);
			max_y = get_max_y_of_card(i);
			max_x = get_max_x_of_card(i);
			if ((t_x >= min_x && t_x <= max_x) && (t_y >= min_y && t_y <= max_y)) {
				set_flipped(i + 1);
				return;

			}
		}
	}
}

/*-[Reshape Callback]--------------------------------------------------------*/
void reshapeFunc(int x, int y) {
	if (y == 0 || x == 0) return;  //Nothing is visible then, so return

	glMatrixMode(GL_PROJECTION); //Set a new projection matrix
	glLoadIdentity();
	//Angle of view: 40 degrees
	//Near clipping plane distance: 0.5
	//Far clipping plane distance: 20.0

	gluPerspective(40.0, (GLdouble)x / (GLdouble)y, 0.5, 40.0);
	glViewport(0, 0, x, y);  //Use the whole window for rendering
}

void idleFunc(void) {

}


int main(int argc, char** argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH);
	glutInitWindowPosition(500, 500); //determines the initial position of the window
	glutInitWindowSize(800, 600);	  //determines the size of the window
	windowid = glutCreateWindow("Our Fourth OpenGL Window"); // create and name window

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);

	// register callbacks
	initTextures();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(onMouseClick);
	glutReshapeFunc(reshapeFunc);
	glutIdleFunc(idleFunc);

	// GLUT Full Screen
	//glutFullScreen();

	glutMainLoop(); // start the main loop of GLUT
	return 0;
}

