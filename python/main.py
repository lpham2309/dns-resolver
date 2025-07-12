from python import DNSResolver, resolve_domain

if __name__=="__main__":
    try:
        resolver = DNSResolver()
        input_domain = None

        while(True):
            input_domain = input("Enter the domain name (or 'exit' to quit): ")
            if input_domain == "exit":
                break
            domain_ips = resolve_domain(input_domain)
    except ValueError:
        print("Invalid domain input")