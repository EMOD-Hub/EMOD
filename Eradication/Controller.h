
#pragma once
#include <string>
#include <fstream>
#include <functional>

#include "IController.h"
#include "ISimulation.h"
#include "Environment.h"

/*
** The Controller class serves as the bridge between the Simulation and the outside environment
** It handles reading configuration files and creating the simulation object. It is also the locus
** of implementation of high level simulation plans involving averaging, serialization, etc
*/

class DefaultController : public IController
{
public:
    virtual bool Execute();
    virtual ~DefaultController() {}

protected:
    bool execute_internal();
};

