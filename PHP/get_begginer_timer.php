<?php
  include 'database.php';
  
  //---------------------------------------- Condition to check that POST value is not empty.
  if (!empty($_POST)) {
    // Keep track post values
    $myObj = (object)array();
    
    //........................................
    $pdo = Database::connect();
    $sql = "SELECT Connection_Time FROM tb_config WHERE ID = (SELECT MAX(ID) FROM tb_config)";

    $q = $pdo->prepare($sql)

    // Executa a consulta e verifica sucesso
    if ($q->execute()) {
      $result = $q->fetch(PDO::FETCH_ASSOC); // Busca uma linha como array associativo

      if ($result) {
          // Popula o objeto de resposta
          $myObj->Connection_Time = $result['Connection_Time'];

          // Retorna a resposta como JSON
          echo json_encode($myObj);
      } else {
          // Nenhum resultado encontrado
          echo json_encode(array("error" => "No Connection Time found in the database."));
      }
  } else {
      // Falha na execução da consulta
      echo json_encode(array("error" => "Failed to execute query."));
  }
    
    Database::disconnect();
    //........................................ 
  }
  //---------------------------------------- 
?>

