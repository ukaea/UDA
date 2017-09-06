function s=getData(name, source, varargin)
% s=getData(name, source, ['plot'])
% reads IDAM data, returns a structure (q.v.)
% the structure will always contain an item 'ErrorCode', which is zero if the data is successfully got

% validate the input parameters
error(nargchk(2, 3, nargin))
if ~isstr(name), error('The first parameter needs to be a string, the name of the data item'); end
if ~isstr(source), source=num2str(source); end

% call the Mexfile built from getIdamData.c
% In view of the variability of the data structure, it is easier to get the whole lot in a cell array then sort it out here

c=getIdamData(name, source);

n=1;
ndim=1;
while n<=length(c) & ~isempty(c{n})	
	switch lower(c{n})
	case 'name'
		s.Name=c{n+1};
		n=n+1;
	case 'source'
		s.Source=c{n+1};
		n=n+1;
	case 'errorcode'
		s.ErrorCode=c{n+1};
		n=n+1;
	case 'errormessage'
		s.ErrorMessage=c{n+1};
		n=n+1;
	case 'order'
		s.Order=c{n+1} + 1;
		n=n+1;
	case 'rank'
		s.Rank=c{n+1};
		n=n+1;
	case 'data'
		s.DataLabel=c{n+1};
		s.DataUnits=c{n+2};
		s.Data=c{n+3};		
		n=n+3;
    case 'error'
		s.ErrorLabel=c{n+1};
		s.ErrorUnits=c{n+2};
		s.Error=c{n+3};		
		n=n+3;		
        
	case 'dimension'
		s.Dimension{ndim}.Label=c{n+1};
		s.Dimension{ndim}.Units=c{n+2};
		s.Dimension{ndim}.Data=c{n+3};		
		ndim=ndim+1;
		n=n+3;
	otherwise
		fprintf(1, 'Unrecognised key found: %s\n', c{n});
	end
	n=n+1;
end

if isfield(s, 'Order') & isfield(s, 'Rank') & isfield(s, 'Dimension')
	if (s.Rank==1) & (s.Order==1)
		s.Time=s.Dimension{1}.Data;
		s.TimeUnits=s.Dimension{1}.Units;
		s.TimeLabel=s.Dimension{1}.Label;
		% For some reason this sometimes gives an error
		% s=rmfield(s, 'Dimension');
	end
	
	if s.Rank>1
		sz=zeros(1, s.Rank);
		for n=1:s.Rank
			sz(n)=length(s.Dimension{n}.Data);
		end		
		s.Data=reshape(s.Data, sz);
	end	

	if any(strcmp(varargin, 'plot')) & isfield(s, 'Time') & isfield(s, 'Data')
		figure
		plot(s.Time, s.Data)
                
        xlabel([s.TimeLabel ' ' s.TimeUnits], 'Interpreter', 'none')
		ylabel([s.DataLabel ' ' s.DataUnits], 'Interpreter', 'none')
		title([s.Name ' ' s.Source], 'Interpreter', 'none')
        
         
	end	

end

