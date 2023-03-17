export default {
  maxTxDataLength: 3500 - 4, // 4 bytes in header (src station/net, dst station/net)
  maxScoutExtraDataLength: 32 - 6, // 6 bytes in header (src station/net, dst station/net, control byte, port)
};
