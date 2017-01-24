<?php

// Test interaction with a Postgresql database

// Connection

$database = "doi";
$host     = "localhost";
$port     = "56566";
$dbuser   = "readonly";
$password = "readonly";

$arg = "dbname=".$database." user=".$dbuser." password=".$password." host=".$host." port=".$port;

echo $arg;

echo "<p>Has a tcp socket connection from this machine to the database host <b>".$host."</b> been entered in the
pg_hba.conf file?</p>";

$conn = pg_connect($arg) or die ("Unable to connect!"); 

// Test Connection

// Execute some SQL

$sql = "select * from doi_table;";

$result = pg_exec($conn, $sql) or die ("SQL bad!");

// Test the outcome

// Results

$nrows   = pg_num_rows($result);
$nfields = pg_num_fields($result);

echo "<p>Number of rows  : $nrows </p>";
echo "<p>Number of fields: $nfields </p>";

if($nrows == 0 || $nfields == 0){
   echo "<p>No results selected!</p>"; 
}

// Print the result set

echo "<table border=1>\n";
for($lt = 0; $lt < $nrows; $lt++) {
   echo "<tr>\n";
   for($gt = 0; $gt < $nfields; $gt++) {
      echo "<td>" . pg_result($result, $lt, $gt) . "</td>\n";
   }
   echo "</tr>\n";
}
echo "</table>\n";

// Close the connection

pg_close($conn);

?>


