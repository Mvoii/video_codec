#!/bin/bash
# start backend server
cd backend || exit
python app.py &
P1=$!

# start front end server
cd ../frontend || exit
npm run preview &
P2=$!

# wait for both processses
wait $P1 $P2
