/*
   Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
   This file is licensed under the Apache License, Version 2.0 (the "License").
   You may not use this file except in compliance with the License. A copy of
   the License is located at
    http://aws.amazon.com/apache2.0/
   This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied. See the License for the
   specific language governing permissions and limitations under the License.
*/
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>

 extern "C" int func(char*bucket, char*object, int *count, char **buffer);
 
/**
 * Get an object from an Amazon S3 bucket.
 */
int func(char*bucket, char*object, int *count, char **buffer)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        const Aws::String bucket_name = bucket;
        const Aws::String key_name = object;
	
        Aws::S3::S3Client s3_client;

        Aws::S3::Model::GetObjectRequest object_request;
        object_request.WithBucket(bucket_name).WithKey(key_name);

        auto get_object_outcome = s3_client.GetObject(object_request);

        if (get_object_outcome.IsSuccess())
        {
	    Aws::OFStream local_file;
            local_file.open(key_name.c_str(), std::ios::out | std::ios::binary);
            local_file << get_object_outcome.GetResult().GetBody().rdbuf();
            std::cout << "Done!" << std::endl;
	    
	    *count = local_file.tellp();
	    std::cout << *count << std::endl;
	    
	    *buffer = (char *)malloc(*count*sizeof(char));
	    FILE *fd = fopen(key_name.c_str(), "rb");
	    	  
	    fread(*buffer, *count, sizeof(char), fd);
            fclose(fd);	    
        }
        else
        {
            std::cout << "GetObject error: " <<
                get_object_outcome.GetError().GetExceptionName() << " " <<
                get_object_outcome.GetError().GetMessage() << std::endl;
        }
    }

    Aws::ShutdownAPI(options);
}
