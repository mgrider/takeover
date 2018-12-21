// TAKEOVER
// by Martin Grider
//
// super-simple game, 2-6 players
//
// Setup: Everyone starts with X number of blinks with one face in their player color. You may play with
//        additional empty blinks, or not.
// 
// Gameplay: Take turns, and either add or move one of your blinks on the gameboard.
//           Your color will take over the adjacent faces of any attached blinks if 
//           a) that face is empty, or
//           b) your blink has more faces in your color than the opposing blink has faces
//              in the opposing color.
// Notes:    - Once it's been added to the board, a blink must be attached to all 
//             other blinks in play at the end of a player's turn. 
//           - You may not move more than one blink on a turn. It doesn't matter which one you move.
// 
// Game Over: The game ends when one player has successfully taken over all faces of a single blink.
//

enum GameState {
  PLAYING,
  GAMEOVER,
  RESETTING
};
byte currentGameState = PLAYING;
Timer gameStateTimer;

const byte FACESTATE_EMPTY = -1;
const byte FACESTATE_P1 = 1;
const byte FACESTATE_P2 = 2;
const byte FACESTATE_P3 = 3;
const byte FACESTATE_P4 = 4;
const byte FACESTATE_P5 = 5;
const byte FACESTATE_P6 = 6;
const byte FACESTATE_RESETTING = 63;

byte currentFacePlayers[] = { -1, -1, -1, -1, -1, -1 };

// note that white is not used (these correspond to player indexes)
Color playerColors[] = {WHITE, RED, BLUE, YELLOW, GREEN, CYAN, MAGENTA};


/// utility methods

byte strengthForPlayer(byte player) {
  byte strength = 0;
  for (int i=0; i<FACE_COUNT; i++) {
    if (currentFacePlayers[i] == player) {
      strength = strength + 1;
    }
  }
  return strength;
}


/// changing game state

void resetGame() {
  currentGameState = RESETTING;
  setValueSentOnAllFaces(FACESTATE_RESETTING);
  gameStateTimer.set(400);
}

void startGame() {
  currentGameState = PLAYING;
  gameStateTimer.set((100*100*100));
  setValueSentOnAllFaces(FACESTATE_EMPTY);
}

void doPlayerSetup() {
  setValueSentOnAllFaces(FACESTATE_EMPTY);
  if (currentFacePlayers[0] == FACESTATE_EMPTY) {
    currentFacePlayers[0] = FACESTATE_P1;
  }
  else {
    currentFacePlayers[0] = currentFacePlayers[0] + 1;
    if (currentFacePlayers[0] > FACESTATE_P6) {
      currentFacePlayers[0] = FACESTATE_P1;
    }
  }
  setValueSentOnFace(currentFacePlayers[0], 0);
  for (int i=1; i < FACE_COUNT; i++) {
    currentFacePlayers[i] = FACESTATE_EMPTY;
  }
}


void setup() {
  startGame();
}

void loop() {

  if (buttonDoubleClicked()) {
    if(isAlone()) {
      // player setup (only if nobody is near)
      FOREACH_FACE(f) {
        doPlayerSetup();
      }
    }
    else {
      // otherwise, we are resetting the game
      resetGame();
    }
  }

  if (currentGameState == RESETTING) {
    return;
  }
  else if (currentGameState == GAMEOVER) {
    return;
  }

  // check each face for a takeover
}
