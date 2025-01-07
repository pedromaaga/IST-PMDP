<?php
  include 'database.php';

  $response = (object)array(); // Initialize response object

  if (!empty($_POST)) {
  $pdo = Database::connect();
  $sql = "SELECT Connection_Time FROM tb_config WHERE ID = (SELECT MAX(ID) FROM tb_config)";
  $q = $pdo->prepare($sql);

  if ($q->execute()) {
    $result = $q->fetch(PDO::FETCH_ASSOC);

    if ($result && isset($result['Connection_Time'])) {
      $response->Connection_Time = (float)$result['Connection_Time'];
    } else {
      $response->Connection_Time = -1; // Return null if no data is found
    }
  } else {
    $response->error = "Query execution failed.";
  }

  Database::disconnect();
  }

  // Return JSON response
  header('Content-Type: application/json');
  echo json_encode($response);

?>
