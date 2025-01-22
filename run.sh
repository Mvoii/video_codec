#!/bin/bash
# start backend server
cd backend || exit
python -m flask run &
P1=$!

# start front end server
cd ../frontend || exit
npm run dev &
P2=$!

# wait for both processses
wait $P1 $P2
