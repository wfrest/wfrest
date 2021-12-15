wrk.method = "POST"

wrk.body  = '{"userId": "10001", "coinType": "GT", "type": "2","amount": "5.1"}'

wrk.headers["Content-Type"] = "application/json"

function request()
  return wrk.format('POST', nil, nil, body)
end
