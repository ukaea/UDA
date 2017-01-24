#include <iostream>
#include <cstdio>
#include <idamclientserverpublic.h>

#include "Idam.hpp"

struct coilGeometryType {
    int id ;
    int count ;
    double turnCount[63] ;
    double rCentre[63] ;
    double zCentre[63] ;
    double dR[63] ;
    double dZ[63] ;
    double angle1[63] ;
    double angle2[63] ;
};

int main()
{
    int test = 1;

    //Idam::Client::setServerHostName("localhost");
    //Idam::Client::setServerPort(56564);

    Idam::Client::setServerHostName("idam0");
    Idam::Client::setServerPort(56561);

    Idam::Client client;

    //-----------------------------------------------------------------------------------
    // Basic data access

    if (test == 1 || test == 0) {

        const Idam::Result& data = client.get("ip", "13500");

        // Check for errors

        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        std::cout << data.rank() << std::endl;
        std::cout << data.size() << std::endl;

        std::cout << data.type().name() << std::endl;
        std::cout << data.label() << std::endl;
        std::cout << data.units() << std::endl;
        std::cout << data.description() << std::endl;

        const Idam::Dim dim = data.dim(0);

        std::cout << dim.num() << std::endl;
        std::cout << dim.size() << std::endl;

        std::cout << dim.type().name() << std::endl;
        std::cout << dim.label() << std::endl;
        std::cout << dim.units() << std::endl;

        if (test != 0) return 0;
    }

    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    // Access Meta Data (Arbitrary Hierarchical structures are returned)
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    // The last MAST shot

    if (test == 2 || test == 0) {

        const Idam::Result& data = client.get("meta::getdata(context=data, device=MAST, /lastshot)", "MAST::");

        // Check for errors

        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (i.e. register) the data tree for accessors

        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        // Cast to a known structure type: check the structure name and version is correct

        Idam::TreeNode root = data.tree();
        root.print(); // Print the Tree Node details

        // node containing data with a specific structure type
        Idam::TreeNode node = root.findStructureDefinition("DATALASTSHOT");
        node.print(); // Print the Tree Node details

        // If the structure definition is known and located, a simple cast is all that is required
        // Check the number of structures is one

        int count = node.parent().numChildren();

        struct DATALASTSHOT {
            unsigned int lastshot;
        };
        typedef struct DATALASTSHOT DATALASTSHOT;

        DATALASTSHOT *datalastshot = (DATALASTSHOT *)node.data();

        printf("\nstructure array count = %d\n\n", count);
        printf("\nlast shot = %d\n\n", (int) datalastshot->lastshot);

        // Alternatively, assuming the structure definition is volatile, extract the atomic
        // scalar component directly
        // Drill down targeting the Atomic type component using its name
        // Check it's not an array

        // Target the first child node with the named variable target
        node = root.findStructureComponent("lastshot");

        node.print();
        node.printUserDefinedTypeTable();

        int acount = node.atomicCount();

        std::cout << "Number of Atomic structure items" << std::endl;
        std::cout << acount << std::endl;

        std::vector<std::string> anames = node.atomicNames();
        std::vector<std::string> atypes = node.atomicTypes();
        std::vector<bool> apoint = node.atomicPointers(); // is this component a Pointer
        std::vector<std::size_t> arank  = node.atomicRank();
        std::vector<std::vector<std::size_t> > ashape = node.atomicShape();

        for (int i=0; i < acount; i++) {
            fprintf(stdout, "%s [%s] %d %zu\n",
                    anames[i].c_str(), atypes[i].c_str(), bool(apoint[i]), arank[i]);

            if (anames[i] == "lastshot"
                    && atypes[i] == "unsigned int"
                    && !apoint[i]
                    && (arank[i] == 0 || (arank[i] == 1 && ashape[i][0] == 1))) {
                unsigned int lastshot = *reinterpret_cast<unsigned int*>(node.structureComponentData(anames[i]));
                printf("\nlast shot = %u\n\n", lastshot);
                break;
            }
        }

        // Call a high level method for the data

        Idam::Scalar lastshot = root.atomicScalar("lastshot"); // Start from the ROOT node

        printf("\nRC = %d\n\n", lastshot.isNull());
        printf("\ntype = %s\n", lastshot.type().name());
        printf("\nlast shot = %u\n\n", lastshot.as<unsigned int>());

        lastshot = node.atomicScalar("lastshot"); // Start from a tree node other than the root

        printf("\nRC = %d\n\n", lastshot.isNull());
        printf("\ntype = %s\n", lastshot.type().name());
        printf("\nlast shot = %u\n\n", lastshot.as<unsigned int>());

        if (test != 0) return 0;
    }

    //-----------------------------------------------------------------------------------
    // List signals available (ROW cast)

    if (test == 3 || test == 0) {

        const Idam::Result& data = client.get("meta::listdata(context=data,device=MAST,shot=22812)", "MAST::");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        // Find the data node

        Idam::TreeNode node = root.findStructureComponent("data.datalistsignals.signal_name"); // node containing data

        node.print();

        // Find the data node

        // node containing data with a specific structure type
        node = root.findStructureDefinition("DATALISTSIGNALS_R");

        node.print();

        // Target the first child node with the named variable target
        node = root.findStructureComponent("signal_name");

        node.printUserDefinedTypeTable();

        // If the structure definition is known and located, a simple cast is all that is required

        std::vector<Idam::TreeNode> children = node.parent().children();
        printf("\nstructure array count = %zu\n\n", children.size());

        // print each signal name
        // the returned data is an array of structures - loop over each array member and print the signal name

        for (std::vector<Idam::TreeNode>::iterator iter = children.begin(); iter != children.end(); ++iter) {
            char * signal_name = reinterpret_cast<char *>(iter->structureComponentData("signal_name"));
            printf("%s\n", signal_name);
        }

        if (test != 0) return 0;
    }

    //=======================================================================================================
    // List signals available (COLUMN cast - more efficient)

    if (test == 4 || test == 0) {

        const Idam::Result& data = client.get("meta::listdata(context=data,cast=COLUMN,device=MAST,shot=22812)", "MAST::");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        // Find the data node

        // node containing data with a specific structure type
        Idam::TreeNode node = root.findStructureComponent("data.signal_name");

        node.print();

        // Find the data node

        // node containing data with a specific structure type
        node = root.findStructureDefinition("DATALISTSIGNALS_C");

        node.print();

        // Target the first child node with the named variable target
        node = root.findStructureComponent("signal_name");

        node.printUserDefinedTypeTable();

        // If the structure definition is known and located, a simple cast is all that is required
        // Check the number of structures is one

        int count = node.parent().numChildren();
        printf("\nstructure array count = %d\n\n", count);

        struct DATALISTSIGNALS_C {
            unsigned int count;
            unsigned int shot;
            int pass;
            char **signal_name;
            char **generic_name;
            char **source_alias;
            char **type;
            char **description;
        };

        DATALISTSIGNALS_C * sdata = reinterpret_cast<DATALISTSIGNALS_C *>(node.data());

        printf("\nstructure array count = %d\n\n", sdata->count);

        for (unsigned int i = 0; i < sdata->count; ++i) {
            printf("%s\n", sdata->signal_name[i]);
        }

        // Target the structure member

        int acount = node.atomicCount();

        std::cout << "Number of Atomic structure items" << std::endl;
        std::cout << acount << std::endl;

        std::vector<std::string> anames = node.atomicNames();
        std::vector<std::string> atypes = node.atomicTypes();
        std::vector<bool> apoint = node.atomicPointers(); // is this component a Pointer
        std::vector<std::size_t> arank = node.atomicRank();

        for (int i = 0; i < acount; ++i) {
            fprintf(stdout, "%s [%s] %d %zu\n", anames[i].c_str(), atypes[i].c_str(), bool(apoint[i]), arank[i]);
        }

        int signalListCount = *reinterpret_cast<int *>(node.structureComponentData("count"));

        char ** signalList = reinterpret_cast<char **>(node.structureComponentData("signal_name"));

        for (int i = 0; i < signalListCount; ++i) {
            printf("%s\n", signalList[i]);
        }

        // Use the High level method

        std::vector<char *> signalVec = node.atomicVector("signal_name").as<char *>();

        for (std::vector<char *>::iterator iter = signalVec.begin(); iter != signalVec.end(); ++iter) {
            printf("%s\n", *iter);
        }

        if (test != 0) return 0;
    }

    //=============================================================================================================
    // Code configuration signal set names

    if(test == 5 || test == 0) {

        struct SIGNALSET {
            int type;
            char name[16];
            char classname[16];
            char system[16];
            char subSystem[16];
            char configuration[20];
        };

        const Idam::Result& data = client.get("meta::getdata(context=meta,device=MAST,cast=column,class=code,system=specview,configuration=default,/latest)", "MAST::22812");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        root.printStructureNames();

        // node containing data with a specific structure type
        Idam::TreeNode node = root.findStructureDefinition("SARRAY.root.SIGNALSET");

        // node containing data with a specific structure type
        node = root.findStructureDefinition("SIGNALSET");

        node.printUserDefinedTypeTable();

        std::vector<Idam::TreeNode> signals = node.parent().children();
        int signalSetCount = signals.size();

        printf("\nSIGNALSET structure array count = %zu\n\n", signals.size());

        int acount = node.atomicCount();

        std::cout << "Number of Atomic structure items" << std::endl;
        std::cout << acount << std::endl;

        std::vector<std::string> anames = node.atomicNames();
        std::vector<std::string> atypes = node.atomicTypes();
        std::vector<bool> apoint = node.atomicPointers(); // is this component a Pointer
        std::vector<std::size_t> arank = node.atomicRank();

        for (int i = 0; i < acount; i++) {
            fprintf(stdout, "%s [%s] %d %zu\n", anames[i].c_str(), atypes[i].c_str(), bool(apoint[i]), arank[i]);
        }

        // Loop over all child nodes (structure array elements) and print the atomic data from each

        char * name;
        char * className;
        char * system;
        char * subSystem;
        char * configuration;

        int i = 0;
        for (std::vector<Idam::TreeNode>::iterator iter = signals.begin(); iter != signals.end(); ++iter) {
            name = iter->atomicScalar("name").as<char *>();
            className = iter->atomicScalar("class").as<char *>();
            system = iter->atomicScalar("system").as<char *>();
            subSystem = iter->atomicScalar("subSystem").as<char *>();
            configuration = iter->atomicScalar("configuration").as<char *>();
            fprintf(stdout, "[%d] name=%s, class=%s, system=%s, subsystem=%s, configuration=%s\n",
                    i++, name, className, system, subSystem, configuration);
        }

        // Cast to known type and print

        SIGNALSET *signalSet = static_cast<SIGNALSET *>(signals[0].data());
        std::cout << std::endl;

        for (i = 0; i < signalSetCount; i++) {
            printf("[%d] name=%s, classname=%s, system=%s\n",
                   i, signalSet[i].name, signalSet[i].classname, signalSet[i].system);
        }

        // Use the High level method

        std::vector<char *> names = node.atomicVector("name").as<char *>();
        std::vector<char *> classNames = node.atomicVector("class").as<char *>();
        std::vector<char *> systems = node.atomicVector("system").as<char *>();
        std::vector<char *> subSystems = node.atomicVector("subSystem").as<char *>();
        std::vector<char *> configurations = node.atomicVector("configuration").as<char *>();

        std::cout << std::endl;
        for (i = 0; i < signalSetCount; i++) {
            printf("[%d] name=%s, classname=%s, system=%s\n", i, names[i], classNames[i], systems[i]);
        }

        if(test != 0) return 0;

    }

//=============================================================================================================
// Code configuration signal set names

    if(test == 6 || test == 0) {

        const Idam::Result& data = client.get("meta::getdata(context=meta,device=MAST,cast=column,class=code,system=specview,configuration=default,/latest)", "MAST::22812");

        Idam::TreeNode root = data.tree();

        // node containing data with a specific structure type
        Idam::TreeNode node = root.findStructureDefinition("SIGNALSET");

        std::vector<Idam::TreeNode> signals = node.parent().children();
        int signalSetCount = signals.size();

        struct SIGNALSET {
            int type;
            char name[16];
            char classname[16];
            char system[16];
            char subSystem[16];
            char configuration[20];
        };

        // Cast to Array of structures
        SIGNALSET * signalSet = static_cast<SIGNALSET *>(signals[0].data());

        struct COORDINATE {
            float r;
            float z;
            float phi;
        };

        struct COIL {
            char signal[14];
            char orientation[15];
            short polarity;
            float calibration;
            int number;
            unsigned short dataStatus;
            COORDINATE coordinate;
        };

        COIL * coilSet;
        COORDINATE * coord;

        VLEN * specviewCoilSet = (VLEN *)calloc(signalSetCount, sizeof(VLEN));

        printf("\nCOIL size = %zu\n", sizeof(COIL));
        printf("COORDINATE size = %zu\n\n", sizeof(COORDINATE));

        for (int i = 0; i < signalSetCount; i++) {
            char signal[1024];
            sprintf(signal,
                    "meta::getdata(context=meta,device=MAST,class=%s,system=%s,subsystem=%s,configuration=%s,/latest)",
                    signalSet[i].classname, signalSet[i].system, signalSet[i].subSystem, signalSet[i].configuration);

            const Idam::Result& data2 = client.get(signal, "MAST::22812");

            // Check for errors
            if (data2.errorCode() != Idam::OK) {
                std::cout << data2.error() << std::endl;
                return data2.errorCode();
            }

            // Check this returned data is a structure: Set (register) the data tree for accessors
            if (!data2.isTree()) {
                std::cout << "Data is not a Structure when expected!" << std::endl;
                return 1;
            }

            root = data2.tree();

            root.printStructureNames();

            // node containing data with a specific structure type
            node = root.findStructureDefinition("SARRAY.root.COIL");

            if (i == 0) {
                node.printUserDefinedTypeTable();
                node.printUserDefinedTypeTable("COORDINATE");
            }

            std::vector<Idam::TreeNode> coils = node.parent().children();
            int count = coils.size();
            printf("\nCOIL structure array count = %d\n\n", count);

            COIL * coilSet = (COIL *)malloc(count * sizeof(COIL));

            specviewCoilSet[i].data = (void *)coilSet;
            specviewCoilSet[i].len  = count;

            for (int j = 0; j < count; j++) {
                char * signal = coils[j].atomicScalar("signal").as<char *>();
                char * orientation = coils[j].atomicScalar("orientation").as<char *>();
                short polarity = coils[j].atomicScalar("polarity").as<short>();
                float calibration = coils[j].atomicScalar("calibration").as<float>();
                int number = coils[j].atomicScalar("number").as<int>();
                unsigned short dataStatus = coils[j].atomicScalar("dataStatus").as<unsigned short>();

                strcpy(coilSet[j].signal, signal);
                strcpy(coilSet[j].orientation, orientation);
                coilSet[j].polarity    = polarity;
                coilSet[j].calibration = calibration;
                coilSet[j].number      = number;
                coilSet[j].dataStatus  = dataStatus;

                COORDINATE * coord = static_cast<COORDINATE *>(coils[j].child(0).data());
                coilSet[j].coordinate = * coord;

                printf("\n[%d] \n", j);
                printf("A   R=%f, Z=%f, Phi=%f\n", coord->r, coord->z, coord->phi);
                printf("B   R=%f, Z=%f, Phi=%f\n",
                       coilSet[j].coordinate.r, coilSet[j].coordinate.z, coilSet[j].coordinate.phi);
            }

            //  Using a cast to a known type ....
            coilSet = static_cast<COIL *>(coils[0].data());

            for (int j = 0; j < count; j++) {
                coord = static_cast<COORDINATE *>(coils[j].child(0).data());

                printf("\n[%d] \n", j);
                printf("A   R=%f, Z=%f, Phi=%f\n", coord->r, coord->z, coord->phi);

                coilSet[j].coordinate = * coord;
                printf("B   R=%f, Z=%f, Phi=%f\n",
                       coilSet[j].coordinate.r, coilSet[j].coordinate.z, coilSet[j].coordinate.phi);
            }
        }

        for (int i = 0; i < signalSetCount; i++) {
            for (int j = 0; j < specviewCoilSet[i].len; j++) {
                coilSet = static_cast<COIL *>(specviewCoilSet[i].data);
                printf("[%2d] [%2d] signal = %s, (R,Z,Phi) = (%f,%f,%f)\n",
                       i, j, coilSet[j].signal,
                       coilSet[j].coordinate.r, coilSet[j].coordinate.z, coilSet[j].coordinate.phi);
            }
        }

        if(test != 0) return 0;
    }


    //=============================================================================================================
    // Integer array tests

    if (test == 7 || test == 0) {

        struct TEST11 {
            int value;
        };

        const Idam::Result& data = client.get("TESTPLUGIN::test11()", "MAST::");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        root.printStructureNames();

        Idam::TreeNode node = root.findStructureDefinition("SARRAY.TEST11");

        node = root.findStructureDefinition("TEST11");

        node.printUserDefinedTypeTable();

        // Check only 1 structure

        int count = node.parent().numChildren();

        printf("\nTEST11 structure array count = %d\n\n", count);

        int acount = node.atomicCount();

        std::cout << "Number of Atomic structure items" << std::endl;
        std::cout << acount << std::endl;

        std::vector<std::string> anames = node.atomicNames();
        std::vector<std::string> atypes = node.atomicTypes();
        std::vector<bool> apoint = node.atomicPointers(); // is this component a Pointer
        std::vector<std::size_t> arank = node.atomicRank();
        std::vector<std::vector<std::size_t> > ashape = node.atomicShape();

        for (int i = 0; i < acount; i++) {
            fprintf(stdout,"%s [%s] %d %zu\n",
                    anames[i].c_str(), atypes[i].c_str(), bool(apoint[i]), arank[i]);
        }

        // Call a high level method for the data

        Idam::Vector vec = node.atomicVector("value");

        std::vector<int> idata  = vec.as<int>();

        for (std::size_t i = 0; i < idata.size(); i++) {
            printf("value[%zu] = %d\n", i, idata[i]);
        }

        if (test != 0) return 0;
    }


    if (test == 8 || test == 0) {

        struct TEST12 {
            int value[3];
        };

        const Idam::Result& data = client.get("TESTPLUGIN::test12()", "MAST::");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        root.printStructureNames();

        Idam::TreeNode node = root.findStructureDefinition("SARRAY.TEST12");

        node = root.findStructureDefinition("TEST12");

        node.printUserDefinedTypeTable();

        // Check only 1 structure

        int count = node.parent().numChildren();
        printf("\nTEST12 structure array count = %d\n\n", count);

        int acount = node.atomicCount();

        std::cout << "Number of Atomic structure items" << std::endl;
        std::cout << acount << std::endl;

        std::vector<std::string> anames = node.atomicNames();
        std::vector<std::string> atypes = node.atomicTypes();
        std::vector<bool> apoint = node.atomicPointers(); // is this component a Pointer
        std::vector<std::size_t> arank = node.atomicRank();
        std::vector<std::vector<std::size_t> > ashape = root.atomicShape();

        for (int i = 0; i < acount; i++) {
            fprintf(stdout, "%s [%s] %d %zu %zu\n",
                    anames[i].c_str(), atypes[i].c_str(), bool(apoint[i]), arank[i], ashape[i][0]);
        }

        // Call a high level method for the data

        Idam::Vector vec = node.atomicVector("value");

        std::vector<int> idata  = vec.as<int>();

        for (std::size_t i = 0; i < vec.size(); i++) {
            printf("value[%zu] = %d\n", i, idata[i]);
        }

        if (test != 0) return 0;
    }

    //========================================================================================================
    // Test 9: IMAS IDS data access from a private file

    if(test == 9 || test == 0) {

        // Read the whole IDS file
        const Idam::Result& data = client.get("/", "NETCDF::/home/dgm/IDAM/test/source/plugins/imas/ids_123_1.hd5");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        root.printStructureNames();

        // target a data tree node containing data with a specific structure type
        Idam::TreeNode node = root.findStructureDefinition("SARRAY.root");

        // node containing data with a specific structure type
        node = root.findStructureDefinition("root");
        node.printUserDefinedTypeTable();

        // node containing data with a specific structure type
        node = root.findStructureDefinition("magnetics");
        node.printUserDefinedTypeTable();

        // node containing data with a specific structure type
        node = root.findStructureDefinition("flux_loop");
        node.printUserDefinedTypeTable();

        // node containing data with a specific structure type
        node = root.findStructureDefinition("13");
        node.printUserDefinedTypeTable();

        // node containing data with a specific structure type
        node = root.findStructureDefinition("flux");
        node.printUserDefinedTypeTable();

        // *** Searching by Type may fail as types are auto-generated to be unique.
        // *** Searching without hierarchical structure will return the first data node satisfying the passed name - which may not be what is needed!
        // *** Searching by structure component name should be a more robust method

        // node containing data with a specific component name
        node = root.findStructureComponent("magnetics.flux_loop.13.flux");
        node.printUserDefinedTypeTable();

        if(test != 0) return 0;
    }

    //========================================================================================================
    if(test == 10 || test == 0) {

        struct FLUX {
            float *data;
            float *time;
        };

        const Idam::Result& data = client.get("/magnetics/flux_loop/13/flux",
                "NETCDF::/home/dgm/IDAM/test/source/plugins/imas/ids_123_1.hd5");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        root.printStructureNames();

        // The first node has no name other tha the generic 'data'
        Idam::TreeNode node = root.findStructureComponent("data");

        node.printUserDefinedTypeTable();

        // Check only 1 structure

        int count = node.parent().numChildren();
        printf("\nTest 10 structure array count = %d\n\n", count);

        int acount = node.atomicCount();

        std::cout << "Number of Atomic structure items" << std::endl;
        std::cout << acount << std::endl;

        std::vector<std::string> anames = node.atomicNames();
        std::vector<std::string> atypes = node.atomicTypes();
        std::vector<bool> apoint = node.atomicPointers(); // is this component a Pointer
        std::vector<std::size_t> arank = node.atomicRank();
        std::vector<std::vector<std::size_t> > ashape = root.atomicShape();

        for (int i=0; i<acount; i++) {
            fprintf(stdout,"%s [%s] %d %zu %zu\n",
                anames[i].c_str(), atypes[i].c_str(), bool(apoint[i]), arank[i], ashape[i][0]);
        }

        // Call a high level method for the data

        std::vector<float> time = node.atomicVector("data.time").as<float>();

        for (size_t i=0; i<time.size(); i++) {
            printf("value[%zu] = %f\n", i, time[i]);
        }

        if (test != 0) return 0;
    }

    //========================================================================================================
    if(test == 11 || test == 0) {

        struct POSITION {
            float *phi;
            float *rValues;
            float *zValues;
        };

        const Idam::Result& data = client.get("/magnetics/flux_loop/13/position",
                "NETCDF::/home/dgm/IDAM/test/source/plugins/imas/ids_123_1.hd5");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        // Locate the data required

        Idam::TreeNode node = root.findStructureComponent("2");

        node.printUserDefinedTypeTable();

        // Check only 1 structure

        int count = node.parent().numChildren();
        printf("\nTest 11 structure array count = %d\n\n", count);

        int acount = node.atomicCount();

        std::cout << "Number of Atomic structure items" << std::endl;
        std::cout << acount << std::endl;

        std::vector<std::string> anames = node.atomicNames();
        std::vector<std::string> atypes = node.atomicTypes();
        std::vector<bool> apoint = node.atomicPointers(); // is this component a Pointer
        std::vector<std::size_t> arank = node.atomicRank();
        std::vector<std::vector<std::size_t> > ashape = root.atomicShape();

        for (int i=0; i<acount; i++) {
            fprintf(stdout,"%s [%s] %d %zu %zu\n",
                anames[i].c_str(), atypes[i].c_str(), bool(apoint[i]), arank[i], ashape[i][0]);
        }

        // Call a high level method for the data

        std::vector<float> vals = node.atomicVector("rValues").as<float>();

        for (std::size_t i=0; i<vals.size(); i++) {
            printf("value[%zu] = %f\n", i, vals[i]);
        }

        if (test != 0) return 0;
    }

    //========================================================================================================
    if (test == 12 || test == 0) {  // return structures of known type
        const Idam::Result& data = client.get("/input/pfSystem/pfCoilsGeometry",
                "NETCDF::/home/dgm/IDAM/test/source/plugins/imas/efitOut.nc");

        // Check for errors
        if (data.errorCode() != Idam::OK) {
            std::cout << data.error() << std::endl;
            return data.errorCode();
        }

        // Check this returned data is a structure: Set (register) the data tree for accessors
        if (!data.isTree()) {
            std::cout << "Data is not a Structure when expected!" << std::endl;
            return 1;
        }

        Idam::TreeNode root = data.tree();

        // Call a high level method for the data

        std::vector<coilGeometryType *> pfCoils = root.structData("data").as<coilGeometryType>();

        // or just extract what's needed

        // returns data from the first structure array element
        std::vector<double> turnCount = root.atomicVector("data.turnCount").as<double>();

        printf("turnCount[%d][%d] = %f   %f\n", 0, 0, turnCount[0], pfCoils[0]->turnCount[0]);

        if(test != 0) return 0;
    }

    return 0;
}
