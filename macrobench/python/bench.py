import json 
import os 
import threading 
import sys 
from memory_profiler import profile

files = []

@profile
def thread_fnc(arrayfiles):
    for i, file in enumerate(arrayfiles):
        #print("File = "+str(file))
        if "transmi" not in file:
            with open(file,"r",encoding="utf-8-sig") as read_file:
            
                json_string=json.load(read_file)
                print("Finished encoding "+str(file)+"\n10 First element "+str(len(json_string)))
        else:
            res = []
            data = [json.loads(line) for line in open(file, 'r',encoding="utf-8-sig")]
            res.append(data)
            print("Finished encoding "+str(file)+"\n10 First element "+str(len(res)))


directory="Files"
default_threads=1 # remember to taskset the program 

def main():
    #hover on the list of available files 
   
    for filename in os.listdir(directory):
        f = os.path.join(directory, filename)
        # checking if it is a file
        if os.path.isfile(f):
            files.append(f)
    
    nb_files = len(files)

    #simple division among threads
    #each thread gets nb_files/thread files
    threads_to_use = default_threads if len(sys.argv)<2 else int(sys.argv[1])

    start=0
    stop=0
    step=nb_files//threads_to_use
    threads = []
    for x in range(0,threads_to_use):
        stop = start+step 
        argthread = files[start:(stop-1)]
        x = threading.Thread(target=thread_fnc,args=(argthread,))
        x.start()
        threads.append(x)
        start = start+stop 
     

    for t in threads:
        t.join()

    return


if __name__ == "__main__":
    main()




