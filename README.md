# dome-daq
Dome DAQ

mhttpd configuration
--------------------

- first execution fails - it need to modify two ODB keys)
```
odbedit -c "set '/Experiment/http redirect to https' n"
odbedit -c "set '/Experiment/midas https port' 0"
```

- configure mserver to accept DAQ events from network

```
odbedit -c "set '/Experiment/Security/Enable non-localhost RPC' y"
odbedit -c "set '/Experiment/Security/Disable RPC hosts check' y"
```

