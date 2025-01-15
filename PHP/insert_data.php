<?php
require 'database.php';

//---------------------------------------- Condition to check that POST value is not empty.
if (!empty($_POST)) {
    // Retrieve and validate POST values
    $temperature = filter_var($_POST['temperature'], FILTER_VALIDATE_FLOAT);
    $humidity = filter_var($_POST['humidity'], FILTER_VALIDATE_FLOAT);
    $light = filter_var($_POST['light'], FILTER_VALIDATE_FLOAT);
    $water = filter_var($_POST['water'], FILTER_VALIDATE_FLOAT);

    if ($temperature !== false && $humidity !== false && $light !== false && $water !== false) {
        // Format values to match DECIMAL(9,2)
        $temperature = number_format($temperature, 2, '.', '');
        $humidity = number_format($humidity, 2, '.', '');
        $light = number_format($light, 2, '.', '');
        $water = number_format($water, 2, '.', '');

        // Insert data into the database
        $pdo = Database::connect();
        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        $sql = "INSERT INTO tb_sensors (Air_Temperature, Soil_Humidity, Light_Intensity, Water_Level) VALUES (?, ?, ?, ?)";
        $q = $pdo->prepare($sql);
        $q->execute(array($temperature, $humidity, $light, $water));
        Database::disconnect();

        // Response to indicate success
        echo json_encode(["status" => "success", "message" => "Data inserted successfully"]);
    } else {
        // Response to indicate invalid input
        echo json_encode(["status" => "error", "message" => "Invalid input data"]);
    }
} else {
    // Response to indicate missing POST data
    echo json_encode(["status" => "error", "message" => "No data provided"]);
}
?>
