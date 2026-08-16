//Imports. Express, filesystem, path.
const express = require('express');
const fs = require('fs');
const path = require('path');
const mongoose = require('mongoose');


const app = express();
const PORT = 3000;

app.use(express.json());
app.use(express.static('public'));

mongoose.connect('mongodb://127.0.0.1:27017/HumidityAndTemperatureStorage')
    .then(() => console.log("Connection Established"))
    .catch(err => console.error('Connection Failed'));

const readingSchema = new mongoose.Schema({
    Index: Number,
    Temperature: Number,
    Humidity: Number,
    Timestamp: Number

})
const batchSchema = new mongoose.Schema({
    readings: [readingSchema],
    receivedAt: {type: Date, default: Date.now}
})

const Batch = mongoose.model('Batch', batchSchema);

app.post('/data/readings', async (req, res) => {
    const batchData = req.body;
    console.log('Batch got');

    try {
        const newBatch = new Batch(batchData);
        await newBatch.save()
        console.log('Saved batch to Database')
    }   catch (err) {
        console.error('Issue saving batch')
    }
    const dir = path.join(__dirname, 'received_readings');
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir);
    }

    const filename = `batch_${Date.now()}.json`;
    const filepath = path.join(dir, filename);

    fs.writeFile(filepath, JSON.stringify(batchData, null, 2), (err) => {
        if (err) {
            console.error('Failed to save file:', err);
            return res.status(500).send({ status: 'Error saving'});
        }
        console.log('Saved readings');
        res.status(200).send({ status: 'Success', message: 'Readings got'});
    });
});
app.listen(PORT, () => {
    console.log(`Listening on port ${PORT}`);
});
//Fetcher for front end
app.get('/api/readings', async (req, res) => {
    try {
        const batches = await Batch.find().sort({ receivedAt: 1 });
        let allReadings = [];
        
        batches.forEach(batch => {
            batch.readings.forEach(r => {
                allReadings.push({
                    temperature: r.Temperature / 10,
                    humidity: r.Humidity / 10,
                    timestamp: new Date(r.Timestamp * 1000).toLocaleTimeString()
                });
            });
        });

        res.json(allReadings);
    } catch (err) {
        console.error('Cannot get readings:', err);
        res.status(500).send({ error: 'Query Failure' });
    }
});
