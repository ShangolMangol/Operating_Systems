import time
import requests
from threading import BoundedSemaphore, Thread

# Global variables
request_limit = 3
max_queue_size = 6
request_counter = 0
queue = []

# Semaphore to control concurrent requests
semaphore = BoundedSemaphore(request_limit)

# Function to send the GET request
def send_request():
    global request_counter
    global queue
    
    # Acquire a semaphore to start the request
    semaphore.acquire()

    request_counter += 1
    request_id = request_counter

    print("Sending request " + str(request_id))

    try:
        # Send the request
        response = requests.get("http://192.168.254.129:7555/output.cgi", timeout=5)
        
        # Simulate the request taking 5 seconds
        time.sleep(5)

        print("Request " + str(request_id) + " completed with status code " + str(response.status_code))

    except requests.exceptions.RequestException as e:
        print("Request " + str(request_id) + " failed: " + str(e))

    # Release the semaphore
    semaphore.release()

    # Remove the request from the queue
    queue.remove(request_id)

# Function to add requests to the queue
def add_request():
    global request_counter
    global queue

    while True:
        # Check if the queue is full
        if len(queue) >= max_queue_size:
            # Drop the oldest request in the queue
            dropped_request = queue.pop(0)
            print("Dropped request " + str(dropped_request))

        # Add the new request to the queue
        request_counter += 1
        queue.append(request_counter)
        print("Added request " + str(request_counter) + " to the queue")

        # Wait for some time before adding the next request
        time.sleep(1)

# Start the request adding thread
request_adder = Thread(target=add_request)
request_adder.start()

# Send multiple requests concurrently
for _ in range(10):
    request_sender = Thread(target=send_request)
    request_sender.start()
