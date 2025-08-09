from DNSResolver import DNSResolver

if __name__=="__main__":
    try:
        resolver = DNSResolver()
        
        while True:
            input_domain = input("Enter the domain name (or 'exit' to quit): ")
            if input_domain == "exit":
                break
            
            print(f"Resolving {input_domain}...")
            ips = resolver.resolve(input_domain)
            
            if ips:
                print(f"IP addresses for {input_domain}:")
                for ip in ips:
                    print(f"  {ip}")
            else:
                print(f"No IP addresses found for {input_domain}")
                
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")