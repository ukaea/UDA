#ifndef UtilsClass
#define UtilsClass

#include <qfile.h>
#include <qstring.h>
#include <qregexp.h>
#include <qdom.h>
#include <vector>
#include <string>
#include <iostream>
#include <gsl/gsl_sort.h>
#include <blitz/array.h>
#include "singleValue.h"
#include <sstream>

//!  Utility routines
namespace UtilitiesNs
{
//! Computes the time-window average at a single data-point given a set of times and values.
/*!
  If the time window extends outside the given data set, then the time window
  is effectively truncated to the given data. For efficiency reasons the the approximate index of timePoint in the times
  array is suppled in a parameter "index".
  \param timeWindow      Size of time-window (secs).
  \param timePoint      Central time about which to perform time-average.
  \param index           Approximate index of timePoint in the times array. If this is not known, suggest put a value close to values.size()/2.
  \param times           Array of times.
  \param values          Array of times-series values.
  \return                Time-window average.
*/
SingleValueNs::SingleValue<double> timeAverage(const SingleValueNs::SingleValue<double> timeWindow, const SingleValueNs::SingleValue<double> timePoint, const SingleValueNs::SingleValue<int> index, const blitz::Array<double,1>& times, const blitz::Array<double,1>& values);




//! Assigns the value of an  XML attribute to a SingleValueNs::SingleValue<double> object.
/*!
  \param DomElement    The Dom element containing the given attribute.
  \param AttributeName The singular name of the attribute.
  \param AttributeValue The variable into which to place the contents of the XML attribute.
  \return If successful returns true, otherwise false
*/
bool getAttributeValue(const QDomElement& domElement, const std::string& attributeName, SingleValueNs::SingleValue<double>& attributeValue);


//! Assigns the value of an  XML attribute to a SingleValueNs::SingleValue<int> object.
/*!
  \param DomElement    The Dom element containing the given attribute.
  \param AttributeName The singular name of the attribute.
  \param AttributeValue The variable into which to place the contents of the XML attribute.
  \return If successful returns true, otherwise false
*/
bool getAttributeValue(const QDomElement& domElement, const std::string& attributeName, SingleValueNs::SingleValue<int>& attributeValue);


//! Assigns the value of an  XML attribute to a blitz::Array<double,1> object.
/*!
  The routine will first try to
  read data with the  singular name provided. If there is nothing found, then
  it will look for data with the plural version of the \e attributeName (by appending
  a "s" to its name).
  \param DomElement    The Dom element containing the given attribute.
  \param AttributeName The singular name of the attribute.
  \param AttributeValue The variable into which to place the contents of the XML attribute.
  \return If successful returns true, otherwise false
*/
bool getAttributeValue(const QDomElement& domElement, const std::string& attributeName, blitz::Array<double,1>& attributeValue);


//! Assigns the value of an  XML attribute to a blitz::Array<int,1> object.
/*!
  The routine will first try to
  read data with the  singular name provided. If there is nothing found, then
  it will look for data with the plural version of the \e attributeName (by appending
  a "s" to its name).
  \param DomElement    The Dom element containing the given attribute.
  \param AttributeName The singular name of the attribute.
  \param AttributeValue The variable into which to place the contents of the XML attribute.
  \return If successful returns true, otherwise false
*/
bool getAttributeValue(const QDomElement& domElement, const std::string& attributeName, blitz::Array<int,1>& attributeValue);


//! Assigns the value of an  XML attribute to a blitz::Array<double,2> object.
/*!
  The routine  will first try to
  read data with the  singular name provided. If there is nothing found, then
  it will look for data with the plural version of the \e attributeName (by appending
  a "s" to its name).
  \param DomElement    The Dom element containing the given attribute.
  \param AttributeName The singular name of the attribute.
  \param AttributeValue The variable into which to place the contents of the XML attribute.
  \return If successful returns true, otherwise false
*/
bool getAttributeValue(const QDomElement& domElement, const std::string& attributeName, blitz::Array<double,2>& attributeValue);


//! Assigns the value of an  XML attribute to a std::string object.
/*!
  \param DomElement    The Dom element containing the given attribute.
  \param AttributeName The singular name of the attribute.
  \param AttributeValue The variable into which to place the contents of the XML attribute (if it  hadn't been specified, then returns "notSet").
  \return  If the variable has been set returns true, otherwise false.
*/
bool getAttributeValue(const QDomElement& domElement, const std::string& attributeName, std::string& attributeValue);

/*!
  \file utilities.h
  \brief contains global functions.
*/


//! get the version of EFIT++ being run
/*!
  This function returns a string describing the version of EFIT being run. If it is a release,
  the string returned will be something like "1.0" or "1.4" etc. If it is in the development branch
  the string will be something like "development/main" (for the main development banch) or
  "development/nmercadier" for a subranch in the development thread.
*/
std::string getEfitVersion();

//! Returns a "NaN" into a floating point double variable type.
/*!
  To generate a NaN  we take a floating point number greater than one, assign it to a variable
  and repeatedly multiply the variable by itself and stick the result back into the variable.
  This is done until the new value is equal to the previous value. At that point you have positive
  infinity.  Nw we divid the number by itself, and the result is a "quiet" NaN.
  \return value NaN.
*/
double getNaN() ;



//! Function opens an XML file, parses it and returns a reference to the DOM document.
bool openAndParseXmlFile(const std::string& xmlFileName, QDomDocument& domDocument);






/////////////////////////////////////////////////////////////////////////////
//
//  valueToString converts a scalar of any type to a std::string.
//
/*!
  \param value     The scalar value to be converted to a std::string.
  \return          Std::String value.
*/
template <class T> std::string valueToString( T value )
{
    std::ostringstream oss;

    oss << value;

    return oss.str();
}



/////////////////////////////////////////////////////////////////////////////
//
//  convertStringToValue converts a QString or Std::String scalar variable to an integer. There
//  are a set of three overloaded functions.
//

//! Overloaded template function converts a scalar of type Typ (permissible types are std::string or QString) to an integer scalar.
/*!
  \param stringValue   The scalar value to be converted.  If entered as type \e std::string it is converted to type \e QString
  \param datavalue If conversion is successful then on exit contains the result.
  \return          If not successful contains the value "integer", otherwise it is empty.
*/
template <class Typ> std::string convertStringToValue(const Typ& stringValue, int& intValue) {
    bool ok;
    std::string expectedType="";
    QString qStringValue=stringValue;
    intValue= qStringValue.toInt(&ok)   ;
    if (ok  == false) expectedType="integer";
    return expectedType;
}

//! Overloaded template function converts a scalar of type Typ (permissible types are std::string QString) to a double float scalar.
/*!
  \param stringValue   The scalar value to be converted.  If entered as type \e std::string it is converted to type \e QString
  \param datavalue If conversion is successful then on exit contains the result.
  \return          If not successful contains the value "double", otherwise it is empty.
*/
template <class Typ> std::string convertStringToValue(const Typ& stringValue, double& doubleValue) {
    bool ok;
    std::string expectedType="";
    QString qStringValue=stringValue;
    doubleValue= qStringValue.toDouble(&ok);
    if (ok  == false) expectedType="double";
    return expectedType;
}

//! Overloaded template function "converts" a scalar of type (permissible types are std::string or QString) to a QString scalar (included for completeness).
/*!
  \param stringValue   The scalar value to be "converted".   If entered as type \e std::string it is converted to type \e QString
  \param std::stringOutvalue On exit contains the result.
  \return          On exit is empty (i.e. contains the value "").
*/
template <class Typ> std::string convertStringToValue(const Typ& stringValue, QString& stringOutValue)
{
    std::string expectedType="";
    QString qStringValue=stringValue;
    stringOutValue= qStringValue;
    return expectedType;
}

/////////////////////////////////////////////////////////////////////////////

//
//  convertStringToVector converts a QString variable to a 1-D array of values
//
//! Converts a scalar of type QString to a 1-D array of values.
/*!
  The routine parses \e dataIn using a regular expression to search for a comma- or
  space- separated list. The list may optionally be enclosed in square brackets: [ ].
  Examples of valid std::string formats are as follows:
   -   1.2,3e0,2.
   -   [1.2,3e0,2.]

  \param dataIn          Input std::string. If entered as type \e std::string it is converted to type \e QString.
  \param dataout         Output vector is a 1-D array containing data. The array may be
  allocated prior to entry or within the routine; in any case the array will be
  enlarged if it is too small.
*/
template <class Typ> void convertStringToVector( const QString& dataIn, blitz::Array<Typ,1>& dataOut)
{
    Typ dataValue;
    int index=-1;
    int pos=0 ;

    // This reg expression captures anything between one or more commas and/or spaces
    QRegExp parseVar("\\s*\\[?\\s*,?\\s*"         // "\\s*"=0 or more spaces;  "\\[?"= 0 or 1 [ ; ",?" =0 or 1 comma.
                     "("                          // start capture group
                     "[^\\[\\],\\s]"              // the capture group matches anything other than "[],"and whitespace
                     "+"                          // can have one or more characters in the capture group.
                     ")"                          // end of capture cgroup
                     ",?\\]?");                   // if possible match: ",?"=0 or 1 comma; "]?"=0 or 1 ].
    dataOut.resize(0);
    while(pos >=0 ) {
        pos=parseVar.search( dataIn, pos );   // search for next item of a comma or space-separated list
        if (pos > -1) {
            // found a values, so convert it to a real number and store it.
            QString dataString=parseVar.cap(1);
            std::string expectedType = convertStringToValue(dataString, dataValue) ;
            // if array needs to be increased in size, then do so, preserving existing contents.
            if(++index >dataOut.size()-1) dataOut.resizeAndPreserve(index+1);
            // and store value
            dataOut(index) = dataValue;
            pos  += parseVar.matchedLength();

        }
    }
}

//
//  convertStringToArray converts a QString variable to a 2-D array of values
//
//! Overloaded template function converts a scalar of type QString to a 2-D array of values.
/*!
  The routine parses \e dataIn using a regular expression to search for a comma- or
  space- separated list. The data in each dimension is separated by square brackets.
  The data is stored in row major order, ie. first row second, third row etc. An
  optional pair of numbers at the start of the std::string can be set to specify the data storage.
  If this is too small, it will be increased during the read-in.  The number of data values
  in each dimension can vary, but any values not explicitly defined will be left unitialized.
  Examples of valid std::string formats are as follows:
    -#      [[1.2 3e0 2.][1.][1. 2. 4. 6.]]
    -#      [[1.2,3e0,2.][1.][1. 2. 4. 6.]]
    -# 3,4  [[1.2,3e0,2.][1.][1. 2. 4. 6.]]
    -# 3,2  [[1.2,3e0,2.][1.][1. 2. 4. 6.]]

  \param dataIn          Input std::string. If entered as type \e std::string it is converted to type \e QString.
  \param dataout         Output Array containing data. The array may be
  allocated prior to entry or within the routine; in any case the array will be
  enlarged if it is too small.
*/
template <class Typ> void convertStringToArray( const QString& dataIn, blitz::Array<Typ,2>& dataOut)
{
    int row=-1;
    int pos=0 ;
    blitz::Array<Typ,1> dataRow;
    blitz::Array<int,1> dataShape;

    // define a regular expression to identify the data "preamble". This may be " [" or " 1 4[". The numbers are optional and define
    // the size of the array
    QRegExp preamble("\\s*"                       // "\\s*"=0 or more spaces.
                     "("                          // start capture group
                     "[^\\[\\]]*"                  // the capture group matches anything other than "[],"and whitespace
                     ")"                          // end of capture group
                     "\\[");                      // match to a [.

    // define a regular expression to identify a single row, eg [[ 1.,2.3.4,6]]
    QRegExp singleRow("\\s*\\[\\s*\\[?"            // "\\s*"=0 or more spaces;  "\\["= [.
                      "("                          // start capture group
                      "[^\\[\\]]"                  // the capture group matches anything other than "[]"
                      "+"                          // can have one or more characters in the capture group.
                      ")"                          // end of capture group
                      "\\]\\s*");                   // if possible match: ",?"=0 or 1 comma; "]?"=0 or 1 ].



    //
    // in the first part of the routine we process the data preamble
    //
    bool outputArraySizeIsSet =false;
    pos=preamble.search( dataIn, pos );   // search for integer list followied by a [; If present, set the intial array dimensions to these values.
    if (pos > -1) {
        // extract the captured std::string, and convert it to numbers.
        QString dataString = preamble.cap(1);
        convertStringToVector(dataString,dataShape);
        if(dataShape.size() > 0) {
            dataOut.resize(dataShape(0),dataShape(1));
            outputArraySizeIsSet =true;
        }
        pos  += preamble.matchedLength();
    }
    //
    // Now process the data itself.
    //
    while(pos >=0 ) {
        pos=singleRow.search( dataIn, pos );   // search for the next row
        if (pos > -1) {
            // found data for a new row so increment row counter.
            row++;
            // extract the captured std::string, and convert it to numbers.
            QString dataString=singleRow.cap(1);
            convertStringToVector(dataString,dataRow);
            // resize the array if required, preserving existing contents.
            if(outputArraySizeIsSet) {
                int columnCount = max(dataRow.size(),dataOut.ubound(blitz::secondDim)+1);
                int rowCount = max(row+1,dataOut.ubound(blitz::firstDim)+1);
                dataOut.resizeAndPreserve(rowCount,columnCount);
            }
            else {
                int columnCount = dataRow.size();
                int rowCount = row+1;
                dataOut.resize(rowCount,columnCount);
                outputArraySizeIsSet =true;
            }

            // assign values in this row to dataOut.
            dataOut(row,blitz::Range(0,dataRow.ubound(blitz::firstDim))) = dataRow;
            // move position marker along the orginal string in preparation for the next parse
            pos  += singleRow.matchedLength();
        }
    }
}







//
//  convertStringToArray converts a QString variable to a 1-D array of values
//
//! Overloaded template function converts a scalar of type QString to a 1-D array of values.
/*!
  The routine parses \e dataIn using a regular expression to search for a comma- or
  space- separated list. The data is optionally enclosed in square brackets [] in which case
  the number of data values can be specified to enable the data dimensions to be set before parsing the data.
  The data array will nevertheless be increased in size if necessary to hold all the data.
  Examples of valid std::string formats are as follows:
  -#    1.2,3e0,2.
  -#    [1.2,3e0,2.]
  -# 3  [1.2,3e0,2.]

  \param dataIn          Input std::string. If entered as type \e std::string it is converted to type \e QString.
  \param dataout         Output Array containing data. The array may be
  allocated prior to entry or within the routine; in any case the array will be
  enlarged if it is too small.
*/
template <class Typ> void convertStringToArray( const QString& dataIn, blitz::Array<Typ,1>& dataOut)
{
    int pos=0 ;
    blitz::Array<Typ,1> dataRow;
    blitz::Array<int,1> dataShape;

    // define a regular expression to identify the data "preamble". This may be " [" or " 1 4[". The numbers are optional and define
    // the size of the array
    QRegExp preamble("\\s*"                       // "\\s*"=0 or more spaces.
                     "("                          // start capture group
                     "[^\\[\\]]"                  // the capture group matches anything other than "[],"and whitespace
                     ")"                          // end of capture group
                     "\\[");                      // match to a [.

    // define a regular expression to identify a single row, eg [[ 1.,2.3.4,6]]
    QRegExp singleRow("\\s*\\[?"                  // "\\s*"=0 or more spaces;  "\\["= [.
                      "("                          // start capture group
                      "[^\\[\\]]"                  // the capture group matches anything other than "[]"
                      "+"                          // can have one or more characters in the capture group.
                      ")");                        // end of capture group

    //
    // in the first part of the routine we process the data preamble
    //
    pos=preamble.search( dataIn, pos );   // search for integer list followied by a [; If present, set the intial array dimensions to these values.
    if (pos > -1) {
        // extract the captured std::string, and convert it to numbers.
        QString dataString = preamble.cap(1);
        convertStringToVector(dataString,dataShape);
        if(dataShape.size() > 0) dataOut.resize(dataShape(0));
        pos  += preamble.matchedLength();
    }
    else {
        // didn't find any preamble, so reset position index.
        pos=0;
        dataOut.resize(0);
    }

    //
    // Now process the data itself.
    //
    pos=singleRow.search( dataIn, pos );   // search for the next row
    if (pos > -1) {
        // found data so extract the captured std::string, and convert it to numbers.
        QString dataString=singleRow.cap(1);
        convertStringToVector(dataString,dataOut);
    }
}



//
//  XmlVerifyAndAllocate performs some basic checks on the XML and the storage array, and allocates space if everything is fine
//
/*!
  Performs some basic checks when reading a collection of identical structures (for example magnetic probes or flux loops)
  into a 1-D storage array from an XML file.
  -# If the detectorArray has already been allocated, it checks that the \e id's of
  detectors in the  detectorArrayList are within the range of the detectorArray.
  -# If the detectorArray has not yet been defined, it checks that the \e id's of
  detectors in the  detectorArrayList form a continuous set  1,2,3....
  If everyting is fine and the detectorArray has not yet been allocated it is allocated in this routine.

  \param detectorArrayList  A reference to an array of XML nodes. The data in each node must contain  at least an attribute called \e id.
  \param detectorName       A reference to the detector name, used if there is an error.
  \param detectorArray      A reference to a 1-D array to hold the data. The operation of the routine depends on whether on entry to
  the routine detectorArray is already allocated (see above).
  \return Return bool=\e true if there were no errors otherwise false.
*/
template <class Typ> bool XmlVerifyAndAllocate(const QDomNodeList& detectorArrayList, const std::string& detectorName, blitz::Array<Typ,1>& detectorArray)
{
    // obtain the set of id's
    bool success;
    blitz::Array<int,1> detectorArrayId(detectorArrayList.count());
    for (uint i = 0; i < detectorArrayList.count(); i++) {
        QDomElement detectorArrayElement = detectorArrayList.item(i).toElement();
        detectorArrayId(i) = atoi(detectorArrayElement.attribute("id"));
    }
    int maxIdValue  = blitz::max(detectorArrayId);
    int minIdValue  = blitz::min(detectorArrayId);

    if (detectorArray.size() !=0) {
        // the detectorArray array has already been defined, so check that the id values are in range.
        if(maxIdValue - minIdValue +1 > detectorArray.size() | minIdValue <= 0) {
            std::cout << "Error: An 'id' value of a " << detectorName << " node is out of range." << std::endl;
            std::cout << "       Valid 'id' values must be positive and contiguous." << std::endl;
            std::cout << "      ===    The full list of id's for the " << detectorName << " nodes is:     ===" << std::endl;
            std::cout << detectorArrayId << std::endl;
            return success = false;
        }
        else
            success=true;
    }
    else {
        // the detectorArray array has not yet been defined, so check that the id values form a continuous set.
        // if they do then allocate an array to hold the data.
        int arraySize = detectorArrayId.size();
        if (minIdValue != 1) success=false;
        else {
            gsl_sort_int (detectorArrayId.data(),1, arraySize);
            blitz::Array<int,1> detectorSequence(detectorArrayId(blitz::Range(1,arraySize-1)) - detectorArrayId(blitz::Range(0,arraySize-2)));
            if(any(abs(detectorSequence) != 1)) success=false;
            else success=true;
        }
        if (!success) {
            // the set of id's of the detectorArray do not form a  continuous set.
            std::cout << "Error: The set of 'id's of the " << detectorName << " nodes do not form  continous set between 1 and =" << maxIdValue << std::endl;
            std::cout << "      ===    The full list of id's for the " << detectorName << " nodes is:     ===" << std::endl;
            std::cout << detectorArrayId << std::endl;
            return success = false;
        }
        detectorArray.resize(maxIdValue);
    }
    return success;
}


////////////////////////////////////////////////////////////////

//! template function converts from a 1-D index to multiDimensional indices (Column-major ordering).
/*!
  The routine is useful when wanting to map between 1-D and multi-dimensional script represenation for arrays.
  Fortran arrays are stored in column-major order.

  \param  index         1-D array subscript.
  \param  arrayExtents  Shape of array for which multidimensional scripts are required.
  \return               Array of indices.
*/
template <int rank> blitz::TinyVector<int,rank> convertToMultiDIndicesColumn(const int index, const blitz::TinyVector<int,rank>& arrayExtents)
// converts from a 1-D index to multi-dimensional indices
{
    blitz::TinyVector<int,rank> multiDimensionIndices;
    multiDimensionIndices(0)=index % arrayExtents(0);
    for (int k=1; k < rank; k++) {
        int baseProduct=1;
        for(int j=0; j < k; j++) {
            baseProduct=baseProduct*arrayExtents(j);
        }
        multiDimensionIndices(k)=index/baseProduct % arrayExtents(k);
    }
    return multiDimensionIndices;
}

//! template function converts from a 1-D index to multiDimensional indices (row-major ordering).
/*!
  The routine is useful when wanting to map between 1-D and multi-dimensional script represenation for arrays.
  C arrays are stored in column-major order.

  \param  index         1-D array subscript.
  \param  arrayExtents  Shape of array for which multidimensional scripts are required.
  \return               Array of indices.
*/
template <int rank> blitz::TinyVector<int,rank> convertToMultiDIndicesRow(const int index, const blitz::TinyVector<int,rank>& arrayExtents)
// converts from a 1-D index to multi-dimensional indices
{
    blitz::TinyVector<int,rank> multiDimensionIndices;
    multiDimensionIndices(rank-1)=index % arrayExtents(rank-1);
    for (int k=rank-2; k >= 0; k--) {
        int baseProduct=1;
        for(int j=rank-1; j > k; j--) {
            baseProduct=baseProduct*arrayExtents(j);
        }
        multiDimensionIndices(k)=index/baseProduct % arrayExtents(k);
    }
    return multiDimensionIndices;
}



//! overloaded template function outputs a single variable
/*!
  The routine can be called successively for dumping the data contained in an object.

  \param  isSet          if true then the variable provided has been filled with data.
  \param  parameterName  Text std::string contains the name of the parameter.
  \param  value          The variable containing the data.
*/
template <class Typ> void dumpStructure(const bool isSet, const std::string parameterName, const SingleValueNs::SingleValue<Typ> value)
{
    std::cout.width(25);
    std::cout << std::left << parameterName + " : ";
    if(isSet) {
        std::cout << std::left << value.getVal() <<std::endl;
    }
    else {
        std::cout << std::left << "not set" <<std::endl;
    }
}

//! overloaded template function outputs a single variable
/*!
  The routine can be called successively for dumping the data contained in an object.

  \param  isSet          if true then the variable provided has been filled with data.
  \param  parameterName  Text std::string contains the name of the parameter.
  \param  value          The variable containing the data.
*/
template <class Typ, int rank> void dumpStructure(const bool isSet, const std::string parameterName, const blitz::Array<Typ,rank> value)
{
    std::cout.width(25);
    std::cout << std::left << parameterName + " : ";
    if(isSet) {
        std::cout << value <<std::endl;
    }
    else {
        std::cout << std::left << "not set" <<std::endl;
    }
}

//! overloaded template function outputs a single variable
/*!
  The routine can be called successively for dumping the data contained in an object.

  \param  parameterName  Text std::string contains the name of the parameter.
  \param  value          The variable containing the data.
*/
template <class Typ> void dumpStructure(const std::string parameterName, const blitz::Array<SingleValueNs::SingleValue<Typ>,1> value)
{
    std::cout.width(25);
    std::cout << std::left << parameterName + " : ";
    for (int i=0; i < value.size(); i++ ) {
        if(value(i).isSet()) {
            std::cout << i << "  " << value(i).getVal() <<std::endl;
        }
        else {
            std::cout << i << "  " << std::left << "not set" <<std::endl;
        }
    }
}


//! overloaded template function outputs a single variable
/*!
  The routine can be called successively for dumping the data contained in an object.

  \param  isSet          if true then the variable provided has been filled with data.
  \param  parameterName  Text std::string contains the name of the parameter.
  \param  value          The variable containing the data.
*/
template <class Typ> void dumpStructure(const bool isSet, const std::string parameterName, const Typ value)
{
    std::cout.width(25);
    std::cout <<  std::left << parameterName + " : ";
    if(isSet) {
        std::cout << std::left << value <<std::endl;
    }
    else {
        std::cout << std::left << "not set" <<std::endl;
    }
}

}

#endif


