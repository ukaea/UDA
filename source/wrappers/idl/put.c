// If array passed then use properties of the array: type, rank, shape
// If structure (array) passed then assume form:
//  {data, name}
// Data type - atomic + string

// First Argument: the Plugin directive - a String?

IDL_ENSURE_SCALAR(argv[0]);
IDL_ENSURE_STRING(argv[0]);

//-----------------------------------------------------------------------------------------
// Test the 2nd Argument: Array only or scalar structure
// String arrays are a pain ...

// Array?, data[]

PUTDATA putData;
initIdamPutData(&putData);

PUTDATA_BLOCK_LIST putDataBlockList;
initIdamPutDataBlockList(&putDataBlockList);

char * data = NULL;
int rank = 0, count = 0;
int * shape = NULL;
int type = IDL_TYPE_UNDEF;

if (argc == 2 && argv[1]->flags & IDL_V_ARR && argv[1]->type != IDL_TYP_STRUCT)
{
    IDL_ENSURE_ARRAY(argv[1]);

    if (kw.verbose) {
        fprintf(stdout,"arg #1:Array Passed\n");
    }

    putData.rank  = argv[1]->value.arr->n_dim;
    putData.count = argv[1]->value.arr->n_elts;
    type  = argv[1]->type;
    putData.data  = (char *)argv[1]->value.arr->data;

    if (putData.count == 0) {
        // ERROR
        return (IDL_GettmpLong(-999));
    }

    if (putData.rank > 1) {
        putData.shape = (int *)malloc(rank*sizeof(int));

        for (i=0; i<rank; i++) {
            putData.shape[i] = (int) argv[1]->value.arr->dim[i];
        }
    }

    switch (type) {
    case (IDL_TYPE_BYTE):
        putData.data_type = UDA_TYPE_UNSIGNED_CHAR;
        break;

    //case(IDL_TYPE_STRING):
    //   putData.data_type = UDA_TYPE_CHAR;
    //   break;
    case (IDL_TYPE_UINT):
        putData.data_type = UDA_TYPE_UNSIGNED_SHORT;
        break;

    case (IDL_TYPE_INT):
        putData.data_type = UDA_TYPE_SHORT;
        break;

    case (IDL_TYPE_ULONG):
        putData.data_type = UDA_TYPE_UNSIGNED_INT;
        break;

    case (IDL_TYPE_LONG):
        putData.data_type = UDA_TYPE_INT;
        break;

    case (IDL_TYPE_ULONG64):
        putData.data_type = UDA_TYPE_UNSIGNED_LONG64;
        break;

    case (IDL_TYPE_LONG64):
        putData.data_type = UDA_TYPE_LONG64;
        break;

    case (IDL_TYPE_ULONG64):
        putData.data_type = UDA_TYPE_UNSIGNED_LONG64;
        break;

    case (IDL_TYPE_LONG64):
        putData.data_type = UDA_TYPE_LONG64;
        break;

    case (IDL_TYPE_FLOAT):
        putData.data_type = UDA_TYPE_FLOAT;
        break;

    case (IDL_TYPE_DOUBLE):
        putData.data_type = UDA_TYPE_DOUBLE;
        break;

    case (IDL_TYPE_COMPLEX):
        putData.data_type = UDA_TYPE_COMPLEX;
        break;

    case (IDL_TYPE_DCOMPLEX):
        putData.data_type = UDA_TYPE_DCOMPLEX;
        break;
    }

    int h = idamPutData((char *)IDL_STRING_STR(&(argv[0]->value.str)), &putData);

    if (putData.rank > 0 && putData.shape != NULL) {
        free((void *)putData.shape);
    }

    return (IDL_GettmpLong(h));

} else

    // Passed Structure? {name, data[]}[]   Structures are always defined as an array in IDL

    if (argc == 2 && !(argv[1]->flags & IDL_V_ARR) && argv[1]->type == IDL_TYP_STRUCT)
    {
        IDL_ENSURE_STRUCTURE(argv[1]);

        if (kw.debug) {
            fprintf(stdout,"arg #1:Structure Passed\n");
        }

        count = argv[1]->value.arr->n_elts;   // Shape is ignored

        for (i=0; i<count; i++) {
            addIdamPutDataBlockList((PUTDATA *)argv[1]->value.s.arr->data[i], &putDataBlockList);
        }

        int h = idamPutListData((char *)IDL_STRING_STR(&(argv[0]->value.str)), &putDataBlockList);

        freeIdamPutDataBlockList(putDataBlockList);

        return (IDL_GettmpLong(h));

    }
