const express = require("express");
const bodyParser = require("body-parser");
const app = express();
app.use(express.json());

app.get("/plug_ctrl", function (req, res) {
    res.sendFile(__dirname + '/plug_control.html');
})

app.post("/api/v1/plug_ctrl", function (req, res) {
    console.log(req.body);
});
app.listen(3000, function () {
    console.log("Server started on port 3000");
});