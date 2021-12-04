import gzip
import requests
 
if __name__ == "__main__":
    url = "http://127.0.0.1:8888"
    
    resp = requests.get(url + "/gzip")

    # According to header 'Content-Encoding': 'gzip'
    # gzip data is automatically decompressed by requests
    print(resp.headers)
    print(resp.content)


