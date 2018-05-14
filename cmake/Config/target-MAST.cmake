set( UDA_HOST idam3.mast.ccfe.ac.uk )
set( UDA_PORT 56565 )

add_definitions(
  -DUDA_SERVER_HOST="idam3.mast.ccfe.ac.uk"
  -DUDA_SERVER_HOST2="idam3.mast.ccfe.ac.uk"
  -DUDA_SERVER_PORT=56565
  -DUDA_SERVER_PORT2=56565
  -DAPI_DEVICE="MAST"
  -DAPI_ARCHIVE="MAST"
  -DAPI_PARSE_STRING="::"
  -DAPI_FILE_FORMAT="IDA3"
  -DDEFAULT_ARCHIVE_DIR="/net/fuslsa/data/MAST_Data/"
  -DLOCAL_MDSPLUS_SERVER="idam0"
  -DMAST_STARTPULSE=12270
  -DFORMAT_LEGACY="IDA3"
  -DFORMAT_MODERN="CDF"
  -DSQL_PORT=56566
  -DSQL_DBNAME="idam"
  -DSQL_USER="readonly"
  -DSQL_HOST="idam1.mast.ccfe.ac.uk"
)