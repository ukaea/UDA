
// ToDo: Use the user name within the passed client certificate in the server LOG 


#ifdef SECURITYENABLED
 
int idamServerAuthentication(CLIENT_BLOCK *client_block, SERVER_BLOCK *server_block, unsigned short authenticationStep){
      
   static short initAuthenticationKeys = 1;
      
   static gcry_sexp_t privatekey = NULL;	// Server's
   static gcry_sexp_t publickey  = NULL;	// Client's

   SECURITY_BLOCK *securityBlock = NULL;	// passed between Client and Server with authentication challenges

   ksba_cert_t clientCert=NULL, CACert=NULL;
   
   gcry_error_t errCode;
   int err = 0, rc;
   
   int protocol_id;

//---------------------------------------------------------------------------------------------------------------
// Read the CLIENT_BLOCK and client x509 certificate
   
   if(authenticationStep == 2){

#ifndef TESTIDAMSECURITY
      
 // Receive the client block, respecting earlier protocol versions

      if(debugon){
         fprintf(dbgout,"Waiting for Initial Client Block\n");
	 rc = fflush(dbgout); 
      }
	 
      if (!(rc = xdrrec_skiprecord(serverInput))) {
         err = PROTOCOL_ERROR_5;
	 if(debugon){
	    fprintf(dbgout,"xdrrec_skiprecord error!\n");
	    rc = fflush(dbgout); 
         }	 
         addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 5 Error (Client Block #2)");         
      } else { 

         protocol_id = PROTOCOL_CLIENT_BLOCK;		// Recieve Client Block
      		 
         if((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, NULL, client_block)) != 0){
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 10 Error (Client Block #2)");         
	    if(debugon){
	       fprintf(dbgout,"protocol error! Client Block not received!\n");
	       rc = fflush(dbgout); 
            }
         }
	 	      
         if(debugon && err == 0){
            fprintf(dbgout,"Initial Client Block received\n");
	    printClientBlock(dbgout, *client_block);
	    rc = fflush(dbgout); 
         }	 
      }
      
      if(err != 0) return(err); 
      
// Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)

      xdrrec_eof(serverInput);                  	                    
      
// Protocol Version: Lower of the client and server version numbers
// This defines the set of elements within data structures passed between client and server
// Must be the same on both sides of the socket
// set in xdr_client
   
#endif

      securityBlock = &client_block->securityBlock;
      
      //x509Block.clientCertificate  = securityBlock->client_X509;         	 
      //x509Block.client2Certificate = securityBlock->client2_X509;			// Passed onwards down the server chain

      //x509Block.clientCertificateLength  = securityBlock->client_X509Length; 
      //x509Block.client2CertificateLength = securityBlock->client2_X509Length;        


   }  
    
//---------------------------------------------------------------------------------------------------------------
// Read the Server's Private Key (from a PEM file) and the User's Public Key (from the passed x509 cert)
// Read the Certificate Authority's Public Key to check signature of the client's certificate
// Keys have S-Expressions format

// Key locations are identified from an environment variable
// The client's public key is from a x509 certificate (check date validity + CA signature)

   if(initAuthenticationKeys){
         
      char *env = NULL;
      unsigned short len = 0;

      unsigned char *serverPrivateKeyFile = NULL;
      unsigned char *CACertFile           = NULL; 
      unsigned char *CACertificate        = NULL;  
      unsigned short CACertificateLength  = 0;    
      
      if((env = getenv("IDAM_SERVER_CERTIFICATE")) != NULL){	// Directory with certificates and key files
         len = strlen(env)+56;
	 serverPrivateKeyFile = (unsigned char *)malloc(len*sizeof(unsigned char));
	 CACertFile           = (unsigned char *)malloc(len*sizeof(unsigned char));
	 
	 sprintf(serverPrivateKeyFile,  "%s/serverskey.pem", env);  // Server's
	 sprintf(CACertFile,            "%s/carootX509.der",  env); // CA Certificate for signature verification
      } else {
         err = 999;
         addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err, 
	 "The Server's User Authentication Environment is Incomplete - set the Certificate Directory path!");
	 return err;
      }
             
      do{	// Error Trap

// Read the Client's certificate, check validity, extract the public key 
             
	 if(securityBlock->client_X509 != NULL){
            if((err = makeIdamX509CertObject(securityBlock->client_X509, securityBlock->client_X509Length, &clientCert)) != 0) break; 
            if((err = testIdamX509Dates(clientCert)) != 0) break;				// Check the Certificate Validity
	    if((err = extractIdamX509SExpKey(clientCert, &publickey)) != 0) break;		// get the Public key from an X509 certificate
         }        

// get the server's Private key from a PEM file (for decryption) and convert to S-Expression
       
         if((err = importIdamPEMPrivateKey(serverPrivateKeyFile, &privatekey)) != 0) break;
      
// Read the CA's certificate, check date validity 
    
         if(CACertFile != NULL){
//	    if((err = importIdamSecurityDoc(CACertFile, &CACertificate, &CACertificateLength)) != 0) break; 
//          if((err = makeIdamX509CertObject(CACertificate, CACertificateLength, &CACert)) != 0) break; 
            if((err = importIdamX509Reader(CACertFile, &CACert)) != 0) break;
	    if((err = testIdamX509Dates(CACert)) != 0) break;					
         }     
      
// Test the server's private key for consistency
    
         if((errCode = gcry_pk_testkey (privatekey)) != 0){
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err, "The Server's Private Authentication Key is Invalid!");
            break;
	 }
	 
// Verify the client certificate's signature using the CA's public key 

//err = testCert(CACertFile);
//err = testCert2(CACert);
//err = testCert2(clientCert); 
//exit(1);       


         if((err = idamCheckX509Signature(CACert, clientCert)) != 0) break;

//fprintf(stdout,"\n\n***** client certificate's signature CHECKED OK *****\n\n");

         ksba_cert_release(clientCert);
	 ksba_cert_release(CACert);
	 clientCert = NULL;
	 CACert = NULL;

	 
      } while(0);
      
      if(serverPrivateKeyFile != NULL) free(serverPrivateKeyFile); 
      if(CACertFile != NULL)           free(CACertFile); 
 
      if(CACertificate != NULL) free((void *)CACertificate);
      CACertificate = NULL;
      CACertificateLength = 0;
      
      if(err != 0){      
 	 if(privatekey != NULL) gcry_sexp_release(privatekey);
	 if(publickey  != NULL) gcry_sexp_release(publickey);
	 if(clientCert != NULL) ksba_cert_release(clientCert);	 
	 if(CACert     != NULL) ksba_cert_release(CACert);		// BUG in ksba library# ksba_free !
	 privatekey = NULL;	// These are declared as static so ensure reset when an error occurs
	 publickey  = NULL;	 	 
         return err;
      }
      
      initAuthenticationKeys = 0;
   }   

//---------------------------------------------------------------------------------------------------------------
// Authenticate both Client and Server

   unsigned short encryptionMethod   = ASYMMETRICKEY;
   unsigned short tokenByteLength    = NONCEBYTELENGTH;		// System problem when >~ 110 !
   unsigned short tokenType          = NONCESTRONGRANDOM; 	// NONCESTRINGRANDOM; // NONCEWEAKRANDOM; // NONCETEST; // 

   static gcry_mpi_t client_mpiToken = NULL; 
   static gcry_mpi_t server_mpiToken = NULL; 
   
   unsigned char *client_ciphertext=NULL, *server_ciphertext=NULL;
   unsigned int client_ciphertextLength=0, server_ciphertextLength=0;
   
   do{		// Error Trap

//---------------------------------------------------------------------------------------------------------------
// Step 2: Receive the Client's token cipher (EASP) and decrypt with the server's private key (->A)
   
   if(authenticationStep == 2){      
      
// Already received the encrypted token (A) from the client   
 	 	 
         securityBlock = &client_block->securityBlock;	  
	 
	 if(securityBlock->authenticationStep != authenticationStep-1){
            err = 999;
            addIdamError(&idamerrorstack,CODEERRORTYPE,"idamServerAuthentication",err,"Authentication Step #2 Inconsistency!");
            break;
	 }
	 	       
         client_ciphertext = securityBlock->client_ciphertext;      
         server_ciphertext = securityBlock->server_ciphertext;
	       
         client_ciphertextLength = securityBlock->client_ciphertextLength;     
         server_ciphertextLength = securityBlock->server_ciphertextLength;	        

// Decrypt token (A) 
   
         err = idamAuthentication(authenticationStep, encryptionMethod, 
		                  tokenType,          tokenByteLength,
		                  publickey,          privatekey,
		                  &client_mpiToken,   &server_mpiToken,
		                  &client_ciphertext, &client_ciphertextLength, 
		                  &server_ciphertext, &server_ciphertextLength); 
   
         if(err != 0){
            addIdamError(&idamerrorstack,CODEERRORTYPE,"idamServerAuthentication",err,"Failed Decryption Step #2!");
            break;
         }

	 free((void *)client_ciphertext);
         client_ciphertext = NULL;
         client_ciphertextLength = 0;		             
	 free((void *)server_ciphertext);
         server_ciphertext = NULL;
         server_ciphertextLength = 0;
	 	 		             
      } else
      
//---------------------------------------------------------------------------------------------------------------
// Step 3: Server encrypts the client token (A) with the client's public key (->EACP)

      if(authenticationStep == 3){
            	 
// Encrypt token (A)
   
         err = idamAuthentication(authenticationStep, encryptionMethod, 
		                  tokenType,          tokenByteLength,
		                  publickey,          privatekey,
		                  &client_mpiToken,   &server_mpiToken,
		                  &client_ciphertext, &client_ciphertextLength, 
		                  &server_ciphertext, &server_ciphertextLength); 
   
         if(err != 0){
            addIdamError(&idamerrorstack,CODEERRORTYPE,"idamServerAuthentication",err,"Failed Encryption Step #3!");
            break;
         }

	 securityBlock = &server_block->securityBlock;
         initSecurityBlock(securityBlock);
	 
	 securityBlock->client_ciphertext = client_ciphertext;      	       
         securityBlock->client_ciphertextLength = client_ciphertextLength;     

	 gcry_mpi_release(client_mpiToken);
      
      } else
      
//---------------------------------------------------------------------------------------------------------------
// Step 4: Server issues a new token (B) also encrypted with the client's public key (->EBCP), passes both to client. 

      if(authenticationStep == 4){

// Generate new Token and Encrypt (B)
   
         err = idamAuthentication(authenticationStep, encryptionMethod, 
		                  tokenType,          tokenByteLength,
		                  publickey,          privatekey,
		                  &client_mpiToken,   &server_mpiToken,
		                  &client_ciphertext, &client_ciphertextLength, 
		                  &server_ciphertext, &server_ciphertextLength); 
   
         if(err != 0){
            addIdamError(&idamerrorstack,CODEERRORTYPE,"idamServerAuthentication",err,"Failed Encryption Step #4!");
            break;
         }

// Send the encrypted token to the server 

         securityBlock = &server_block->securityBlock;	   

         securityBlock->authenticationStep      = authenticationStep;      
         securityBlock->server_ciphertext       = server_ciphertext;      
         securityBlock->server_ciphertextLength = server_ciphertextLength;      
      
         protocol_id = PROTOCOL_SERVER_BLOCK;
	  
#ifndef TESTIDAMSECURITY      		 
         if((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, server_block)) != 0){
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err, "Protocol 10 Error (securityBlock #4)");         
	    break;
         }

	 if(!(rc = xdrrec_endofrecord(serverOutput, 1))){
            err = PROTOCOL_ERROR_7;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err, "Protocol 7 Error (Server Block)");         
            break;
	 }  
#endif	 
	 //free((void *)client_ciphertext);
         client_ciphertext = NULL;
         client_ciphertextLength = 0;		             
	 //free((void *)server_ciphertext);
         server_ciphertext = NULL;
         server_ciphertextLength = 0;
	 
	 //initSecurityBlock(securityBlock);		       
      
      } else      

//---------------------------------------------------------------------------------------------------------------
// Step 7: Server decrypts the passed cipher (EBSP) with the server's private key (->B) and checks 
//	   token (B) => client authenticated

      if(authenticationStep == 7){
      
// Receive the encrypted token (B) from the client   
 
         protocol_id = PROTOCOL_CLIENT_BLOCK;
	 	  
#ifndef TESTIDAMSECURITY

         if (!(rc = xdrrec_skiprecord(serverInput))) {
            err = PROTOCOL_ERROR_5;
	    if(debugon){
	       fprintf(dbgout,"xdrrec_skiprecord error!\n");
	       rc = fflush(dbgout); 
            }	 
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 5 Error (Client Block #7)");         
            break;
	 }
      		 
         if((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, NULL, client_block)) != 0){
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err, "Protocol 11 Error (securityBlock #7)");              
	    break;
         } 
	 
// Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)

         xdrrec_eof(serverInput);
      	 
#endif	 
	 securityBlock = &client_block->securityBlock;
	 
	 if(securityBlock->authenticationStep != authenticationStep-1){
            err = 999;
            addIdamError(&idamerrorstack,CODEERRORTYPE,"idamServerAuthentication",err,"Authentication Step Inconsistency!");
            break;
	 }
	 	       
         client_ciphertext = securityBlock->client_ciphertext;      
         server_ciphertext = securityBlock->server_ciphertext;      
         client_ciphertextLength = securityBlock->client_ciphertextLength;     
         server_ciphertextLength = securityBlock->server_ciphertextLength;       

// Decrypt token (B) and Authenticate the Client
   
         err = idamAuthentication(authenticationStep, encryptionMethod, 
		                  tokenType,          tokenByteLength,
		                  publickey,          privatekey,
		                  &client_mpiToken,   &server_mpiToken,
		                  &client_ciphertext, &client_ciphertextLength, 
		                  &server_ciphertext, &server_ciphertextLength); 
   
         if(err != 0){
            addIdamError(&idamerrorstack,CODEERRORTYPE,"idamServerAuthentication",err,"Failed Authentication Step #7!");
            break;
         }
	 
// Send the encrypted token B to the client 

         securityBlock = &server_block->securityBlock;	   

         securityBlock->authenticationStep      = authenticationStep;      
         securityBlock->server_ciphertext       = server_ciphertext;      
         securityBlock->server_ciphertextLength = server_ciphertextLength;      
         securityBlock->client_ciphertext       = client_ciphertext;      
         securityBlock->client_ciphertextLength = client_ciphertextLength;      
      
         protocol_id = PROTOCOL_SERVER_BLOCK;
	  
#ifndef TESTIDAMSECURITY      		 
         if((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, server_block)) != 0){
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err, "Protocol 10 Error (securityBlock #7)");         
	    break;
         }

	 if(!(rc = xdrrec_endofrecord(serverOutput, 1))){
            err = PROTOCOL_ERROR_7;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err, "Protocol 7 Error (Server Block #7)");         
            break;
	 }  
#endif	 

	 //free((void *)client_ciphertext);
         client_ciphertext = NULL;
         client_ciphertextLength = 0;		             
	 //free((void *)server_ciphertext);
         server_ciphertext = NULL;
         server_ciphertextLength = 0;
	 
	 //initSecurityBlock(securityBlock);
	 		             
      } else
      
//---------------------------------------------------------------------------------------------------------------
// Step 9: Housekeeping 

      if(authenticationStep == 9){      
         if(privatekey != NULL) free((void *)privatekey);
         if(privatekey != NULL) free((void *)publickey);
	 privatekey = NULL;
	 publickey  = NULL;
         if(client_mpiToken != NULL) gcry_mpi_release(client_mpiToken);
         if(server_mpiToken != NULL) gcry_mpi_release(server_mpiToken);
         if(client_ciphertext != NULL) free((void *)client_ciphertext);
         if(server_ciphertext != NULL) free((void *)server_ciphertext); 
      }	 	 		               

//---------------------------------------------------------------------------------------------------------------
      
   } while(0);		// End of Error Trap
   
   if(err != 0){
      if(client_mpiToken != NULL) gcry_mpi_release(client_mpiToken);
      if(server_mpiToken != NULL) gcry_mpi_release(server_mpiToken);
      if(client_ciphertext != NULL) free((void *)client_ciphertext);
      if(server_ciphertext != NULL) free((void *)server_ciphertext); 
   }

   return err;
}   

#endif
