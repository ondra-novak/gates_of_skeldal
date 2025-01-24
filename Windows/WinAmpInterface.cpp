// WinAmpInterface.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WAInputPlugin.h"
#include "WaveOut.h"
#include "waplayer.h"


int main(int argc, char* argv[])
{
    WAPlayer player;
    player.LoadPlugins("C:\\Program Files\\Winamp\\Plugins");
    const char *name="\\\\bredy-doma\\d$\\MUSIC\\S3M\\s3m\\CTGOBLIN.S3M";
    WAInputPlugin *plug=player.SelectBestPlugin(name);

	WaveOut output;
	plug->AttachOutput(&output);
    plug->Play(name);
	while (kbhit()==0 && !plug->IsFinished()) Sleep(500);
	plug->Stop();
	plug->AttachOutput(0);



	return 0;
}
