/*
 * launcher.h
 *
 *  Created on: 27. 2. 2026
 *      Author: ondra
 */

#ifndef GAME_LAUNCHER_H_
#define GAME_LAUNCHER_H_

typedef struct launcher_selection {
    const char *ddl_file;   //ddl file = adventure ddl file, can be NULL (standard version)
    const char *lang;       //ddl lang file = cz.ddl, or en.ddl, if NULL, game exits
}TLAUNCHER_SELECTION;


//return allocated struct. You must free pointer when no longer needed
TLAUNCHER_SELECTION *run_launcher();






#endif /* GAME_LAUNCHER_H_ */
