<?php
  include 'database.php';
  
  //---------------------------------------- Condition to check that POST value is not empty.
  if (!empty($_POST)) {
    // keep track post values
    $myObj = (object)array();
    
    //........................................ 
    $pdo = Database::connect();
    $sql = "SELECT End_Time FROM tb_config WHERE ID = (SELECT MAX(ID) FROM tb_config)";
    foreach ($pdo->query($sql) as $row) {
      $myObj->End_Time = $row['End_Time'];
      
      $myJSON = json_encode($myObj);
      
      echo $myJSON;
    }
    Database::disconnect();
    //........................................ 
  }
  //---------------------------------------- 
?>
