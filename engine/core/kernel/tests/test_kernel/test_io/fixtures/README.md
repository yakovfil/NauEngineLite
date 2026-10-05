The legacy fixture retains the first index entry and its exact payload from the pre-change sceneBase content.nau_pack. Its historical uncompressed marker is a single NUL character. The container uses three newlines after Content-Length; the reader consumes two and counts the third with the JSON length. Trailing padding keeps the existing mapped reader page size nonzero.
Source archive SHA256: 898f087cc3ea10bae35370ede02e1ec195ee899323b32baaeb9ebbdd3fe20023
Payload offset: 269
Payload size: 926
