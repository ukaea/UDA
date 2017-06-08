#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test PPF2::help() function", "[PPF2][plugins]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("PPF2::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "PPF2 plugin: Functions Names\n\n"
            "f=PPFGO,shot=n, seq=n\n"
            "f=PDMSEQ,pulse=n,seq=n,owner=s,dda=s\n"
            "f=PPFUID,userid=s,rw=s\n"
            "f=PPFERR,rname=s,error_code=n\n"
            "f=PPFGQI, shot=n\n"
            "f=PPFGID, rw=s\n"
            "f=PPFPOK\n"
            "f=PDAINF\n"
            "f=PPFSEQ, shot=d, ndda=n\n"
            "f=PPFINF, comlen=n, ndda=n\n"
            "f=DDAINF, shot=n, dda=s, nwcom=n, ndt=n\n"
            "f=PDLPPF, shot=n, uid=n, dup=n, nseq=n\n"
            "f=PDLUSR, shot=n, dda=s, nusers=n\n"
            "f=PDMSDT, date=n, time=s\n"
            "f=PDMSHT\n"
            "f=PDSTAT\n"
            "f=PDSRCH, shot=n, dda=s, ndda=n\n"
            "f=PDSTD, shot=n\n"
            "f=PDTINF, shot=n, dda=s, ndt=n\n"
            "f=PPFDAT, shot=n, seq=n, dda=s, dtype=s, nx=d, nt=d\n"
            "f=PPFDTI, shot=n, seq=n, dda=s, dtype=s\n"
            "f=PPFDDA, shot=n, nseq=n, dda=s\n"
            "f=PPFGET, shot=n, dda=s, dtype=s\n"
            "f=PPFGSF, shot=n, seq=n, dda=s, dtype=s\n"
            "f=PPFGTS, shot=n, dda=s, dtype=s, twant=f\n"
            "f=PPFMOD, shot=n\n"
            "f=PPFONDISK, shot=n, dda=s\n"
            "f=PPFOWNERINFO, shot=n, seq=n, flag=n\n"
            "f=PPFSETDEVICE, device=s\n"
      "f=PPFSIZ, shot=n, seq=n, dda=s, user=s\n";

    REQUIRE( str->str() == expected );
}
