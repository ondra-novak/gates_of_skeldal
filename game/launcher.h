/*
 * launcher.h
 *
 *  Created on: 27. 2. 2026
 *      Author: ondra
 */

#ifndef GAME_LAUNCHER_H_
#define GAME_LAUNCHER_H_

#include "game/gamesave.h"


///Return selected launcher item
/**
@note if save is NULL, no automatic save is selected. if ddl is NULL, original adventure is selected. If NULL returned, exit the game
*/
TCONTINUE_GAME_INFO *run_launcher();






#endif /* GAME_LAUNCHER_H_ */
