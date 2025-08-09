from dataclasses import dataclass, field
import numpy as np
import struct

@dataclass
class DNSHeader():
   id: np.uint16
   qr: np.uint16
   opcode: np.uint16
   aa: np.uint16
   tc: np.uint16
   rd: np.uint16
   ra: np.uint16
   z: np.uint16
   rcode: np.uint16
   qdcount: np.uint16
   ancount: np.uint16
   nscount: np.uint16
   arcount: np.uint16

   def to_bytes(self) -> bytes:
      flags = (self.qr << 15) | (self.opcode << 11) | (self.aa << 10) | (self.tc << 9) | (self.rd << 8) | (self.ra << 7) | (self.z << 4) | self.rcode
      return struct.pack('!HHHHHH', self.id, flags, self.qdcount, self.ancount, self.nscount, self.arcount)

@dataclass
class DNSQuestion():
   domain_name: str
   qtype: np.uint16
   qclass: np.uint16

   @property
   def qname(self) -> bytes:
      return resolve_domain(self.domain_name)
   
   def to_bytes(self) -> bytes:
      return self.qname + struct.pack('!HH', self.qtype, self.qclass)
@dataclass
class DNSResolver():
   dns_server: str = "8.8.8.8"  # Google's DNS
   
   def construct_dns_query(self, dns_header: DNSHeader, dns_question: DNSQuestion) -> bytes:
      query = bytearray()
      query.extend(dns_header.to_bytes())
      query.extend(dns_question.to_bytes())

      return bytes(query)
   
   def send_query(self, query: bytes) -> bytes:
      import socket
      sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
      sock.settimeout(5)
      try:
         sock.sendto(query, (self.dns_server, 53))
         response, _ = sock.recvfrom(512)
         return response
      finally:
         sock.close()
   
   def resolve(self, domain: str) -> list[str]:
      # Create DNS header for query
      header = DNSHeader(
         id=1234, qr=0, opcode=0, aa=0, tc=0, rd=1, ra=0, z=0, rcode=0,
         qdcount=1, ancount=0, nscount=0, arcount=0
      )
      
      # Create DNS question (A record query)
      question = DNSQuestion(domain_name=domain, qtype=1, qclass=1)
      
      # Construct and send query
      query = self.construct_dns_query(header, question)
      response_data = self.send_query(query)
      
      # Parse response (simplified - just extract A record IPs)
      return self.parse_response(response_data)
   
   def parse_response(self, response_data: bytes) -> list[str]:
       # Skip header (12 bytes) and question section
      offset = 12
      
      # Skip question section - find end of qname
      while response_data[offset] != 0:
         offset += response_data[offset] + 1
      offset += 5  # Skip null byte + qtype + qclass
      
      # Parse answer records
      ips = []
      header_ancount = struct.unpack('!H', response_data[6:8])[0]
      
      for _ in range(header_ancount):
         # Skip name (assume compression - 2 bytes)
         offset += 2
         
         # Read type, class, ttl, data length
         rtype, rclass, ttl, data_len = struct.unpack('!HHIH', response_data[offset:offset+10])
         offset += 10
         
         # If A record (type 1), extract IP
         if rtype == 1 and data_len == 4:
            ip_bytes = response_data[offset:offset+4]
            ip = '.'.join(str(b) for b in ip_bytes)
            ips.append(ip)
         
         offset += data_len
      
      return ips

@dataclass
class DNSRecord():
   name: str
   type: np.uint16
   class_: np.uint16
   ttl: np.uint32
   data: str

@dataclass
class DNSResponse():
   header: DNSHeader
   questions: list[DNSQuestion] = field(default_factory=list)
   answers: list[DNSRecord] = field(default_factory=list)

def resolve_domain(domain_name: str) -> bytes:
   encoded_domain = b""
   for d in domain_name.split('.'):
      curr_length = len(d)
      encoded_domain += bytes([curr_length]) + d.encode('ascii')
   encoded_domain += b"\x00"
   return encoded_domain
