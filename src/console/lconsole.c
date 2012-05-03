#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/param.h>
#include "launcher.h"


/* --- Defines --- */
#define CHECK_ERROR(e)	{ int _err = e; if (_err) return _err - 1; }


/* --- Types --- */
typedef struct {
	char *path;
	int enumDevs;
} Options;


/* --- Data --- */
Options options;
MlDevice dev;



static int defaultOptions(Options *opts)
{
	opts->path = "/dev/usb/hiddev0";
	opts->enumDevs = 0;
	
	return 0;
}


static void showHelp(const char *progname)
{
	Options opts;
	
	defaultOptions(&opts);
	
	puts  ("Launcher Console " PACKAGE_VERSION "\n");
	puts  ("Usage:\n");
	printf("    %s [options]\n", progname);
	puts  ("\nOptions:\n");
	printf("  -d dev  Path to device (default '%s').\n", opts.path);
	puts  ("  -e      Only enumerate the devices.");
	puts  ("  -h      Show this help.");
	puts  ("  -v      Show program version.");
}


static void showVersion(void)
{
	puts("Launcher Console " PACKAGE_VERSION);
	puts("Copyright (C) 2007, Tommie Gannert");
}


static int parseCmdLine(int argc, char **argv)
{
	int c;
	char *progname;
	
	progname = strrchr(argv[0], '/');
	if (!progname) progname = argv[0];
	else ++progname;
	
	while ((c = getopt(argc, argv, "d:ehv")) != EOF) {
		switch (c) {
		case 'd':
			options.path = optarg;
			break;
			
		case 'e':
			options.enumDevs = 1;
			break;
			
		case 'h':
			showHelp(progname);
			return 1;
			
		case 'v':
			showVersion();
			return 1;
			
		default:
			return 2;
		}
	}
	
	return 0;
}


static int enumerate(void)
{
	MlDeviceEnum devEnum;
	MlDeviceDescr *descr;
	unsigned char buf[sizeof(*descr) + MAXPATHLEN];
	
	if (!options.enumDevs)
		return 0;
	
	if (!(devEnum = mlEnumerateDevices())) {
		fprintf(stderr, "Failed to enumerate devices: %s (%d)\n", strerror(errno), errno);
		return 1;
	}
	
	descr = (MlDeviceDescr*) buf;
	
	while (!mlGetNextDevice(devEnum, descr, sizeof(buf))) {
		char type[64];
		
		printf("  %s: %s\n", descr->path, mlGetDeviceTypeDisplayString(descr->type, type, sizeof(type)));
	}
	
	mlCloseDeviceEnum(devEnum);
	
	return 1;
}


static void cleanup(void)
{
	if (dev) mlCloseDevice(dev);
}


static int openDevice(void)
{
	if (!(dev = mlOpenDevice(options.path))) {
		fprintf(stderr, "Failed to open device '%s': %s (%d)\n", options.path, strerror(errno), errno);
		return 1;
	}
	
	printf("Realigning the turret...\n");
	fflush(stdout);
	if (mlSetUsingFeedback(dev, 1)) {
		fprintf(stderr, "Failed to enable feedback: %s (%d)\n", strerror(errno), errno);
		mlCloseDevice(dev);
		return 1;
	}
	
	atexit(&cleanup);
	
	return 0;
}


static void cmdAim(char **cmd, unsigned int n)
{
	double yaw, pitch;
	
	if (n != 2) {
		fprintf(stderr, "Invalid number of arguments.\n");
		return;
	}
	
	yaw = strtod(cmd[0], NULL);
	pitch = strtod(cmd[1], NULL);
	
	printf("Aiming %.0f deg, inclination %.0f deg... ", yaw, pitch);
	fflush(stdout);
	if (mlAim(dev, yaw, pitch)) {
		fprintf(stderr, "Failed: %s (%d)\n", strerror(errno), errno);
		return;
	}
	
	printf("Done.\n");
}


static void cmdArm(char **cmd, unsigned int n)
{
	printf("Arming... ");
	fflush(stdout);
	if (mlArm(dev)) {
		fprintf(stderr, "Failed: %s (%d)\n", strerror(errno), errno);
		return;
	}
	
	printf("Done.\n");
}


static void cmdCalibrate(char **cmd, unsigned int n)
{
	printf("Calibrating... ");
	fflush(stdout);
	if (mlCalibrate(dev)) {
		fprintf(stderr, "Failed: %s (%d)\n", strerror(errno), errno);
		return;
	}
	
	printf("Done.\n");
}


static void cmdDeltaAim(char **cmd, unsigned int n)
{
	double yaw, pitch;
	
	if (n != 2) {
		fprintf(stderr, "Invalid number of arguments.\n");
		return;
	}
	
	yaw = strtod(cmd[0], NULL);
	pitch = strtod(cmd[1], NULL);
	
	printf("Reaiming %.0f deg, inclination %.0f deg... ", yaw, pitch);
	fflush(stdout);
	if (mlAimDelta(dev, yaw, pitch)) {
		fprintf(stderr, "Failed: %s (%d)\n", strerror(errno), errno);
		return;
	}
	
	printf("Done.\n");
}


static void cmdFire(char **cmd, unsigned int n)
{
	printf("Firing... ");
	fflush(stdout);
	if (mlFire(dev)) {
		fprintf(stderr, "Failed: %s (%d)\n", strerror(errno), errno);
		return;
	}
	
	printf("Done.\n");
}


static void cmdHelp(char **cmd, unsigned int n)
{
	puts("Available commands:\n");
	puts("  aim yaw pitch");
	puts("             Aim at [yaw], [pitch].");
	puts("  arm        Arm one missile.");
	puts("  calibrate  Calibrate the motor timings.");
	puts("  daim yaw pitch");
	puts("             Move aim at [yaw], [pitch].");
	puts("  fire       Fire one missile.");
	puts("  help       Show this help.");
	puts("  info       Show information about the launcher.");
	puts("  load shots Set the number of available shots.");
	puts("  quit       Terminate the console.");
	puts("  realign    Realign the turret.");
	puts("  status     Show status of the launcher.");
	puts("");
}


static void cmdInfo(char **cmd, unsigned int n)
{
	char type[64];
	
	printf("Device Information\n\n");
	printf("ID         : %s\n", options.path);
	printf("Type       : %s\n", mlGetDeviceTypeDisplayString(mlGetDeviceType(dev), type, sizeof(type)));
	printf("Shots      : %d\n", mlGetShotsCount(dev));
	printf("Yaw Range  : %.0f -- %.0f\n", mlGetMinYaw(dev), mlGetMaxYaw(dev));
	printf("Pitch Range: %.0f -- %.0f\n", mlGetMinPitch(dev), mlGetMaxPitch(dev));
	printf("Time Resol.: %.3f s\n", mlGetTimeResolution(dev));
	printf("Yaw Time   : %.2f s\n", mlGetMaxYawTime(dev));
	printf("Pitch Time : %.2f s\n", mlGetMaxPitchTime(dev));
	printf("Arm Time   : %.2f s\n", mlGetArmTime(dev));
	printf("Fire Time  : %.2f s\n", mlGetFireTime(dev));
}


static void cmdLoad(char **cmd, unsigned int n)
{
	unsigned int shots;
	
	if (n != 1) {
		fprintf(stderr, "Invalid number of arguments.\n");
		return;
	}
	
	shots = atoi(cmd[0]);
	
	fflush(stdout);
	if (mlSetAvailableShotsCount(dev, shots)) {
		fprintf(stderr, "Failed to load shots: %s (%d)\n", strerror(errno), errno);
		return;
	}
}


static void cmdRealign(char **cmd, unsigned int n)
{
	printf("Realigning the turret...\n");
	fflush(stdout);
	if (mlRealign(dev)) {
		fprintf(stderr, "Failed: %s (%d)\n", strerror(errno), errno);
		return;
	}
	
	printf("Done.\n");
}


static void cmdStatus(char **cmd, unsigned int n)
{
	double yaw, pitch;
	
	if (mlGetAim(dev, &yaw, &pitch)) {
		fprintf(stderr, "Failed to get aim direction: %s (%d)\n", strerror(errno), errno);
		return;
	}
	
	printf("Device Status\n\n");
	printf("Aiming: %.0f degs, inclination %.0f degs\n", yaw, pitch);
	printf("Armed : %s\n", (mlIsArmed(dev) ? "Yes" : "No"));
	printf("Shots : %d\n", mlGetAvailableShotsCount(dev));
}


static int doCommand(char **cmd, unsigned int n)
{
	char *cmd0 = cmd[0];
	
	if (!strcasecmp(cmd0, "aim"))
		cmdAim(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "arm"))
		cmdArm(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "calibrate"))
		cmdCalibrate(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "daim"))
		cmdDeltaAim(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "fire"))
		cmdFire(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "help"))
		cmdHelp(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "info"))
		cmdInfo(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "load"))
		cmdLoad(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "quit"))
		return 1;
	else if (!strcasecmp(cmd0, "realign"))
		cmdRealign(cmd + 1, n - 1);
	else if (!strcasecmp(cmd0, "status"))
		cmdStatus(cmd + 1, n - 1);
	
	return 0;
}


static char* getPrompt(char *prompt, size_t size)
{
	double yaw, pitch;
	
	if (mlGetAim(dev, &yaw, &pitch)) {
		return "ready> ";
	}
	
	snprintf(prompt, size, "%d shots @ (%4.0f, %4.0f)%s> ",
		mlGetAvailableShotsCount(dev),
		yaw, pitch,
		(mlIsArmed(dev) ? " ARMED" : ""));
	
	return prompt;
}


static int run(void)
{
	char prompt[64];
	char *line;
	
	puts("\nWelcome to Launcher Console " PACKAGE_VERSION);
	puts("Terminate with 'quit' or Ctrl-D. Help is availble using 'help'.\n");
	
	{
		char type[64];
		
		printf("Connected to a %s at '%s'.\n\n",
			mlGetDeviceTypeDisplayString(mlGetDeviceType(dev), type, sizeof(type)),
			options.path);
	}
	
	while ((line = readline(getPrompt(prompt, sizeof(prompt))))) {
		char *cmd[16];
		char *p = line;
		int n = 0;
		
		if (!*line) {
			free(line);
			continue;
		}
		
		add_history(line);
		
		for (p = line; p && n < 16; ++n) {
			cmd[n] = p;
			p = strchr(p, ' ');
			if (p) *(p++) = 0;
		}
		
		if (!*line || !n) {
			free(line);
			continue;
		}
		
		if (doCommand(cmd, n)) {
			free(line);
			break;
		}
		
		free(line);
	}
	
	return 0;
}


int main(int argc, char **argv)
{
	CHECK_ERROR( defaultOptions(&options) );
	CHECK_ERROR( parseCmdLine(argc, argv) );
	CHECK_ERROR( enumerate()              );
	CHECK_ERROR( openDevice()             );
	
	return run();
}
