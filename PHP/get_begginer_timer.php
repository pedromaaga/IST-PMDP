<?php
  include 'database_admin.php';
  
  //---------------------------------------- Condition to check that POST value is not empty.
  if (!empty($_POST)) {
    // Keep track post values
    $myObj = (object)array();
    
    //........................................
    $pdo = Database::connect();
    $sql = "SELECT Connection_Time FROM tb_config WHERE ID = (SELECT MAX(ID) FROM tb_config)";
    
    // Execute query and check if any data is returned
    $result = $pdo->query($sql);
    if ($result && $row = $result->fetch(PDO::FETCH_ASSOC)) {
      // If data is found, return it
      $myObj->Connection_Time = $row['Connection_Time'];
    } else {
      // If no data found, return a default value or message
      $myObj->Connection_Time = "No data available";
    }

    // Convert the result to JSON and return it
    $myJSON = json_encode($myObj);
    echo $myJSON;
    
    Database::disconnect();
    //........................................ 
  }
  //---------------------------------------- 
?>

