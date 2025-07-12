from dataclasses import dataclass, field
import numpy as np

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

@dataclass
class DNSQuestion():
   domain_name: str
   qtype: np.uint16
   qclass: np.uint16

   @property
   def qname(self) -> bytes:
      return encode_domain(self.domain_name)
   
@dataclass
class DNSResolver():
   def construct_dns_query(self, dns_header: DNSHeader, dns_question: DNSQuestion) -> str:
       query = bytearray()
       query.extend(dns_header)
       query.extend(dns_question)

       return query

def resolve_domain(domain_name: str) -> bytes:
   encoded_domain = b""
   for d in domain_name.split('.'):
      curr_length = len(d)
      encoded_domain += bytes(curr_length) + d.encode('ascii')
   encoded_domain + b"\x00"
   return encoded_domain.hex()
