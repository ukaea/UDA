#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test META::help() function", "[META][plugins]" )
{
#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("META::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree()  );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "help" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "STRING" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar value = child.atomicScalar("help");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(char*).name() );
    REQUIRE( value.size() == 0 );

    const char* expected = "\nMETA: The META server side function library provides access to the IDAM Metadata registry. "
            "This registry records different classes of metadata. "
            "Access to these classes depends on the data request context. "
            "There are three contexts: META means the metadata recorded in the registy. DATA means the metadata "
            "recorded in the data respository. And CPF means the high level plasma classification data recorded in "
            "the CPF database. The default context is DATA. "
            "The library provides functions (list and get methods) to retrieve the required information as well as describing "
            "what information is available, enabling Data Discovery.\n\n"

            "Function: list(context={meta|data|cpf} [,{cast|align}={row|column}] [,class=class] [,system=system]\n"
            "               [,{Config|configuration}=configuration] [,device=device]\n"
            "               [,{shot|pulno|exp_number}=shot]\n"
            "               [,version=version] [,revision=revision]\n"
            "               [,{source|alias|source_alias}=source_alias] [,type={A|R|M|P|I|*}] [,pass=pass]\n"
            "               [,description=description] [,table=table]\n"
            "               [,/listclasses] [,/listsources] [,/structure])\n\n"
            "This function is used for data discovery and returns a list of the meta data available "
            "for a specific shot and/or class etc.\n"
            "The meanings of keywords are:\n"
            "\tlistdevices (with context=meta}) - return a list of device names for which metadata exists.\n"
            "\tlistclasses (with context={meta|cpf}) - return a list of metadata classes available.\n"
            "\tlistsources (with context=data) - return a list of data sources metadata available for a specific shot.\n"
            "\tstructure (with context=meta) - return the associated data structure definitions (work in progress!).\n\n"

            "IDL Examples:\n"
            "\tList all available data sources for a given MAST shot\n"
            "\t\tstr=getdata('meta::list(context=data, /listSources)', '17300') or\n"
            "\t\tstr=getdata('meta::list(context=data, shot=17300, /listSources)', '')\n"
            "\tList all available data signals (Raw, Analysed, Modelled, etc.) for a given MAST shot\n"
            "\t\tstr=getdata('meta::list(context=data, shot=23456), type=A', 'MAST::')\n"
            "\tList metadata classes for a specific context\n"
            "\t\tstr=getdata('meta::list(context=meta, /listClasses)','') or\n"
            "\t\tstr=getdata('meta::list(context=cpf, /listClasses)','')\n"
            "\tList all available metadata\n"
            "\t\tstr=getdata('meta::list(context=meta)','')\n"

            "\n\nFunction: get(context={meta|cpf|data} [,{cast|align}={row|column}] [,class=class] [,system=system]\n"
            "                  [,configuration=configuration] [,device=device]\n"
            "                  [,{shot|pulno|exp_number}=shot]\n"
            "                  [,version=version] [,revision=revision]\n"
            "                  [,names=\"var1,var2,var3,...\"] [,table=table] [,where=where] [,limit=limit]\n"
            "                  [,/lastshot] [,/shotdatetime] [,/latest])\n\n"
            "This function is used to return a data structure populated with the metadata requested. If the context is 'meta', "
            "the selection criteria must be sufficient to identify a single metadata registry record, "
            "otherwise the ambiguity will cause an error.\n"
            "The meanings of keywords are:\n"
            "\tlastshot (with context=data) - return the last shot number recorded for the default device.\n"
            "\tshotdatetime (with context=data) - return the date and time of a specific plasma shot for the default device.\n"
            "\tlatest (with context=meta) - return the latest metadata version and revision.\n\n"
            "IDL Examples:\n"
            "\tGet the Soft X-Ray camera configuration data\n"
            "\t\tstr=getdata('meta::get(context=meta,class=diagnostic,system=sxr)', '')\n"
            "\tGet the Omaha magnetic coil names and locations\n"
            "\t\tstr=getdata('meta::get(context=meta,class=magnetic,system=omaha,configuration=specview,version=1,revision=1,shot=17301)', '')\n"
            "\tGet the set of coordinates that map out the limiting surfaces inside the MAST vessel\n"
            "\t\tstr=getdata('meta::get(context=meta,class=structure,configuration=limiter)','')\n"
            "\tGet the last shot number\n"
            "\t\tstr=getdata('meta::get(context=data,/lastshot)','')\n"
            "\tGet the data and time of a shot\n"
            "\t\tstr=getdata('meta::get(context=data,shot=13500,/shotdatetime)','')\n"
            "\tGet plasma classification data from the CPF\n"
            "\t\tstr=getdata('meta::get(context=cpf,names=\"exp_number,tstart,tend\",table=mast0,"
            "where=\"tend>=0.4 ORDER BY exp_number desc\",limit=99,cast=column)','')\n";

    REQUIRE( std::string(expected) == value.as<char*>() );
}
