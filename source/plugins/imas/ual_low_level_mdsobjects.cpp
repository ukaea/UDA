//Suppoprt MDSplus functionality  for the UAL based on MDSobjects


#include <mdsobjects.h>
#include <string.h>

using namespace MDSplus;

extern "C" char * spawnCommand(char *command, char *ipAddress);

static char *doLocalShellCommand(char *command);
static char *doRemoteShellCommand(char *command, char *ipAddress);


char *spawnCommand(char *command, char *ipAddress)
{
    char *answ, *cppAnsw;
    if(!ipAddress || !*ipAddress)
	cppAnsw = doLocalShellCommand(command);
    else
	cppAnsw = doRemoteShellCommand(command, ipAddress);
    if(!cppAnsw) return NULL;
    answ = (char *)malloc(strlen(cppAnsw)+1);
    strcpy(answ, cppAnsw);
    delete[] cppAnsw;
    return answ;
}


static char *doLocalShellCommand(char *command)
{
    try {
	String *commandData = new String(command);
	Data *retAnswer = executeWithArgs("imas->doShellCommand:DSC($)",1, commandData);
	if(!retAnswer) return NULL;
	char *answ = retAnswer->getString();
	deleteData(commandData);
	deleteData(retAnswer);
	return answ;
    }catch(MdsException *exc) {printf("%s\n", exc->what());}
    return NULL;
}

static char *doRemoteShellCommand(char *command, char *ipAddress)
{
    try {
	Connection *conn = new Connection(ipAddress);
	Data **args = new Data *[1];
	args[0] = new String(command);
	const char *cmd = "imas->doShellCommand:DSC($)";
	Data *retAnswer = conn->get(cmd, args, 1);
	if(!retAnswer) return NULL;
	char *answ = retAnswer->getString();
	deleteData(args[0]);
	deleteData(retAnswer);
	delete[] args;
	delete conn;
	return answ;
    }catch(MdsException *exc) {printf("%s\n", exc->what());}
    return NULL;
}

