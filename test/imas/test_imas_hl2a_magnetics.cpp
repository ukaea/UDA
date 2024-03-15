#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_helpers.h"

#include <c++/UDA.hpp>
#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "36908"

/*
* imas::get(expName='HL2A', idx=0, group='magnetics', variable='ids_properties/homogeneous_time', type=int, rank=0, shot=84600, )
bpol_probe/#/field/data
*/
TEST_CASE("Test magnetics/bpol_probe/#/field/data", "[IMAS][HL2A]")
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

	uda::Client client;

 //   const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/data', expName='HL2A', type=float, rank=1, shot=" SHOT_NUM ", )", "");
//	const uda::Result& result = client.get("hl2a::read(idx=0, group='magnetics', variable='bpol_probe/1/field/data',element='bpol_probe/#/field/data', indices='0', expName='HL2A', type=float, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Result& result = client.get("hl2a::read(element = 'magnetics/bpol_probe/#/field/data', indices = '1', experiment = 'HL2A', dtype = 7, shot = " SHOT_NUM ", IDS_version = '')", "");
	printf("return Code = [%d]\r\n", result.errorCode());
	
            REQUIRE( result.errorCode() == 0 );
            REQUIRE( result.errorMessage().empty() );

	uda::Data* data = result.data();

	uda::Dim dim = result.dim(0, uda::Result::DATA);
	
	REQUIRE(data != nullptr);

	
	auto arr = dynamic_cast<uda::Array*>(data);

	std::vector<uda::Dim> dims = arr->dims();

	
	
	REQUIRE(arr != nullptr);
	//REQUIRE(!arr->isNull());

	printf("===============================================================================\r\n");
	printf("Total Data Size is : [%lu]   Dim Size is : [%lu]\r\n", arr->size(), dim.size());
	printf("------------------------------------------------------------------------------\r\n");
	printf("Top 1000 records\r\n");
	printf("------------------------------------------------------------------------------\r\n");
	
	//printf("------------------------------------------------------------------------------\r\n");
	for (int i = 0; i < arr->size(); i++)
	{
		float * val = (float*)arr->byte_data();
		printf("%f ", val[i]);
		if (i >= 1000)
			break;
	}
	printf("===============================================================================\r\n");
}


TEST_CASE("Test magnetics/flux_loop/#/flux/data", "[IMAS][HL2A][FLUX]")
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

	uda::Client client;

//	const uda::Result& result = client.get("hl2a::read(idx=0, group='magnetics', variable='flux_loop/1/flux/data',element='magnetics/flux_loop/#/flux/data', indices='0', expName='HL2A', type=float, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Result& result = client.get("hl2a::read(element = 'magnetics/flux_loop/#/flux/data', indices = '1', experiment = 'HL2A', dtype = 7, shot = " SHOT_NUM ", IDS_version = '')", "");

	printf("return Code = [%d]\r\n", result.errorCode());

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

	uda::Data* data = result.data();

	uda::Dim dim = result.dim(0, uda::Result::DATA);

	REQUIRE(data != nullptr);
	

	auto arr = dynamic_cast<uda::Array*>(data);

	std::vector<uda::Dim> dims = arr->dims();



	REQUIRE(arr != nullptr);
	//REQUIRE(!arr->isNull());

	printf("===============================================================================\r\n");
	printf("Total Data Size is : [%lu]   Dim Size is : [%lu]\r\n", arr->size(), dim.size());
	printf("------------------------------------------------------------------------------\r\n");
	printf("Top 1000 records\r\n");
	printf("------------------------------------------------------------------------------\r\n");

	//printf("------------------------------------------------------------------------------\r\n");
	for (int i = 0; i < arr->size(); i++)
	{
		float * val = (float*)arr->byte_data();
		printf("%f ", val[i]);
		if (i >= 1000)
			break;
	}
	printf("===============================================================================\r\n");
}
