// TAKEOVER
// by Martin Grider
//
// super-simple game, 2-6 players
//
// Setup: Everyone starts with X number of blinks with one face in their player color. 
//        (Double-Click an isolated blink to change its starting color.)
//        You may play with additional empty blinks, or not.
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
// Reset:    Double-Click a group of blinks while playing or after game over to begin again.
//

// TODO:
// - oscilate face colors (using a timer?) (faster for greater strength!)
// - visually show colors from before the game over on button press, maybe?
// - have something happen on single click of an empty blink. Maybe just a very faint (dim) double tap animation.


enum GameState {
  PLAYING,
  PLAYING_WAITING_FOR_TAKEOVER,
  PLAYING_WAITING_FOR_POST_TAKEOVER_DELAY,
  GAMEOVER,
  RESETTING,
  FINISH_RESETTING
};
byte currentGameState = PLAYING;
Timer gameStateTimer;

const byte FACESTATE_EMPTY = 0;
const byte FACESTATE_P1 = 1;
const byte FACESTATE_P2 = 2;
const byte FACESTATE_P3 = 3;
const byte FACESTATE_P4 = 4;
const byte FACESTATE_P5 = 5;
const byte FACESTATE_P6 = 6; // we only allow for 6 players max
const byte FACESTATE_RESETTING = 63;
const byte FACESTATE_GAMEOVER = 62;
const byte MAGIC_DATA_MOD_NUMBER = 7;

byte currentFacePlayers[] = { 0,0,0,0,0,0 };
byte pendingTakeoverFacePlayers[] = { 0,0,0,0,0,0 };

// note that these correspond to FACESTATE/player indexes
Color playerColors[] = {OFF, RED, BLUE, YELLOW, GREEN, CYAN, MAGENTA};

const int timeForStateReset = 400;
const int timeForStateTakeover = 500;

/// utility methods

byte localStrengthForPlayer(byte player) {
  byte strength = 0;
  for (int i=0; i<FACE_COUNT; i++) {
    if (currentFacePlayers[i] == player) {
      strength = strength + 1;
    }
  }
  return strength;
}

byte dataFromStrengthAndPlayer(byte strength, byte player) {
  // note: this has the limitation that it only works for 
  //       strength and player values less than MAGIC_DATA_MOD_NUMBER.
  //       Also, it's possible bit shifting is a better solution.
  return ((strength * MAGIC_DATA_MOD_NUMBER) + player);
}

byte dataGetStrength(byte data) {
  return ((data - (data % MAGIC_DATA_MOD_NUMBER)) / MAGIC_DATA_MOD_NUMBER);
}

byte dataGetPlayer(byte data) {
  return data % MAGIC_DATA_MOD_NUMBER;
}


/// changing game state

void startGame() {
  currentGameState = PLAYING;
  FOREACH_FACE(f) {
    currentFacePlayers[f] = FACESTATE_EMPTY;
    pendingTakeoverFacePlayers[f] = FACESTATE_EMPTY;
  }
  setColor(playerColors[0]);
  gameStateTimer.never();
  setValueSentOnAllFaces(FACESTATE_EMPTY);
}

void resetGame() {
  currentGameState = RESETTING;
  FOREACH_FACE(f) {
    currentFacePlayers[f] = FACESTATE_EMPTY;
    pendingTakeoverFacePlayers[f] = FACESTATE_EMPTY;
  }
  setColor(ORANGE);
  setValueSentOnAllFaces(FACESTATE_RESETTING);
  gameStateTimer.set(timeForStateReset);
}

void doneResetting() {
  currentGameState = FINISH_RESETTING;
  gameStateTimer.set(600);
  setColor(dim(ORANGE, 100));
  setValueSentOnAllFaces(FACESTATE_EMPTY);
}

void showGameOverWithPlayer(byte player) {
  currentGameState = GAMEOVER;
  // maybe don't actually change currentFacePlayers, so we can show them again later?
  setColor(playerColors[player]);
  byte data = dataFromStrengthAndPlayer(6, player);
  setValueSentOnAllFaces(data);
}

void doPlayerSetup() {
  if (currentFacePlayers[0] == FACESTATE_EMPTY) {
    currentFacePlayers[0] = FACESTATE_P1;
  }
  else {
    currentFacePlayers[0] = currentFacePlayers[0] + 1;
    if (currentFacePlayers[0] > FACESTATE_P6) {
      currentFacePlayers[0] = FACESTATE_P1;
    }
  }
  for (int i=1; i < FACE_COUNT; i++) {
    currentFacePlayers[i] = FACESTATE_EMPTY;
  }
}


void setup() {
  startGame();
}

void loop() {

  if (currentGameState == RESETTING) {
    if (gameStateTimer.isExpired()) {
      doneResetting();
    }
    return;
  }
  else if (currentGameState == FINISH_RESETTING) {
    // needed to let all our resetting messages expire
    if (gameStateTimer.isExpired()) {
      startGame();
    }
    return;
  }
  else if (currentGameState == PLAYING_WAITING_FOR_POST_TAKEOVER_DELAY) {
    // This exists because of a race condition in takeovers.
    // See here for details: https://github.com/mgrider/takeover/issues/1
    // Basic strategy is when we see a takeover, we go into
    // state PLAYING_WAITING_FOR_TAKEOVER for 1/2 second, during 
    // which we keep track of new takeovers.
    // After the timer expires, we apply all the changes, sending new
    // player states, but go into state PLAYING_WAITING_FOR_POST_TAKEOVER_DELAY
    // during which we do not accept new takeovers.
    // We wait for another 1/2 seconds, then switch back to the normal PLAYING state.
    if (gameStateTimer.isExpired()) {
      currentGameState = PLAYING;
    }
    else {
      return;
    }
  }
  else if (currentGameState == PLAYING_WAITING_FOR_TAKEOVER && gameStateTimer.isExpired()) {
    FOREACH_FACE(f) {
      if (pendingTakeoverFacePlayers[f] != FACESTATE_EMPTY) {
        currentFacePlayers[f] = pendingTakeoverFacePlayers[f];
      }
      pendingTakeoverFacePlayers[f] = FACESTATE_EMPTY;
    }
    currentGameState = PLAYING_WAITING_FOR_POST_TAKEOVER_DELAY;
    gameStateTimer.set(timeForStateTakeover);
    return;
  }
  else if (buttonDoubleClicked()) {
    if(isAlone()) {
      // player setup (only if nobody is near)
      doPlayerSetup();
      return;
    }
    else {
      // otherwise, reset the game
      resetGame();
      return;
    }
  }

  byte data, player, strength, localPlayer;
  // check each face for a takeover
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { 
      // a neighbor is present on face f
      data = getLastValueReceivedOnFace(f);
      if (data == FACESTATE_RESETTING) {
        resetGame();
        return;
      }
      player = dataGetPlayer(data);
      strength = dataGetStrength(data);
      localPlayer = currentFacePlayers[f];
      if (player == FACESTATE_EMPTY) {
        // do nothing
      }
      else if (localPlayer == FACESTATE_EMPTY || strength > localStrengthForPlayer(localPlayer)) {
        if (currentGameState == PLAYING) {
          currentGameState = PLAYING_WAITING_FOR_TAKEOVER;
          gameStateTimer.set(timeForStateTakeover);
        }
        pendingTakeoverFacePlayers[f] = player;
        if (strength == 6) {
          // game over, man!
          showGameOverWithPlayer(player);
        }
      }
    }
  }

  if (currentGameState == GAMEOVER) {
    // need this here, because we might have set it in the loop above.
    return;
  }

  // finally set the color and value sent
  FOREACH_FACE( f ) {
    // todo: abstract color assignment into a function to oscilate the color fade based on time
    setColorOnFace(playerColors[currentFacePlayers[f]], f);
    data = dataFromStrengthAndPlayer(localStrengthForPlayer(currentFacePlayers[f]), currentFacePlayers[f]);
    setValueSentOnFace(data, f);
  }

}
