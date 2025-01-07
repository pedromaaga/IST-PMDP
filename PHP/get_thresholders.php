<?php
include 'database.php';

if (!empty($_POST)) {
    $week = intval($_POST['week']); // Converte o valor para inteiro

    // Objeto para armazenar a resposta
    $myObj = (object)array();

    // Conexão com o banco de dados
    $pdo = Database::connect();
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    // Consulta SQL
    $sql = "SELECT Soil_Humidity_Min, Light_Intensity_Min, Light_Exposure_Min 
            FROM v_thresholders 
            WHERE ? BETWEEN Start_Week AND End_Week";
    $q = $pdo->prepare($sql);

    // Executa a consulta e verifica sucesso
    if ($q->execute(array($week))) {
        $result = $q->fetch(PDO::FETCH_ASSOC); // Busca uma linha como array associativo

        if ($result) {
            // Popula o objeto de resposta
            $myObj->Soil_Humidity_Min = $result['Soil_Humidity_Min'];
            $myObj->Light_Intensity_Min = $result['Light_Intensity_Min'];
            $myObj->Light_Exposure_Min = $result['Light_Exposure_Min'];

            // Retorna a resposta como JSON
            echo json_encode($myObj);
        } else {
            // Nenhum resultado encontrado
            echo json_encode(array("error" => "No thresholds found for the given week."));
        }
    } else {
        // Falha na execução da consulta
        echo json_encode(array("error" => "Failed to execute query."));
    }

    // Desconecta do banco
    Database::disconnect();
}
?>

