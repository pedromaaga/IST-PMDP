<?php
  include 'database.php';

  $response = (object)array();
  
  //---------------------------------------- Condition to check that POST value is not empty.
  if (!empty($_POST)) {
    $pdo = Database::connect();
    $sql = "SELECT End_Time FROM tb_config WHERE ID = (SELECT MAX(ID) FROM tb_config)";
    $q = $pdo->prepare($sql);

    if ($q->execute()) {
        $result = $q->fetch(PDO::FETCH_ASSOC);

        // Check if there is a result and if 'max_id' exists
        if ($result && isset($result['End_Time'])) {
            $response->End_Time = (float)$result['End_Time'];  // Assign the result to the response object
        } else {
            $response->End_Time = -2;  // Set to -2 if no data is found
        }
    } else {
        $response->error = "Query execution failed.";  // Error message if the query fails
    }

    Database::disconnect();
  }
  //---------------------------------------- 
  // Return the response as JSON
  header('Content-Type: application/json');
  echo json_encode($response);
?>
