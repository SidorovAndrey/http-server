# http-server
Basic http-server using sockets. Project to learn socket programming.
main.c file is used for demo purposes

## How to run
1. Run 'make' in root directory
2. Run 'main' file that appeared by the make command
3. Now it can be tested by curl. Example command:
```
curl http://localhost:8080
```

### Routes feature
Web server supports routes. For now, specified string just returned for exact route added via
```
add_handler("/my-route", func_handler);
```

func_handler must return string.
