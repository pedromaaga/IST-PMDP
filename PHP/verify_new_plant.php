<?php
include 'database.php';

// Initialize response object
$response = (object)array();

// Check if the POST data is not empty
if (!empty($_POST)) {
    // Create a PDO connection
    $pdo = Database::connect();
    $sql = "SELECT MAX(ID) AS max_id FROM tb_config";  // Alias the MAX(ID) as 'max_id'
    $q = $pdo->prepare($sql);

    if ($q->execute()) {
        $result = $q->fetch(PDO::FETCH_ASSOC);

        // Check if there is a result and if 'max_id' exists
        if ($result && isset($result['max_id'])) {
            $response->ID = (float)$result['max_id'];  // Assign the result to the response object
        } else {
            $response->ID = -1;  // Set to -1 if no data is found
        }
    } else {
        $response->error = "Query execution failed.";  // Error message if the query fails
    }

    Database::disconnect();
}

// Return the response as JSON
header('Content-Type: application/json');
echo json_encode($response);
?>
