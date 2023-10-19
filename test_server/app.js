const express = require("express");
const bodyParser = require("body-parser");
const app = express();
app.use(express.json());

app.get("/", function (req, res) {
    res.sendFile(__dirname + '/plug_control.html');
})

app.get("/api/v1/switch", function (req, res) {
    let obj = {switch}
})


app.post("/api/v1/plug_ctrl", function (req, res) {
    console.log(req.body);
});
app.listen(3000, function () {
    console.log("Server started on port 3000");
});