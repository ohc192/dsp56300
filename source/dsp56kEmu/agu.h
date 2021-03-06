#pragma once

#include "types.h"
#include "utils.h"

namespace dsp56k
{
	class AGU
	{
	public:
		static void updateAddressRegister( TWord& r, TWord n, TWord m )
		{
			// linear addressing
			if( m == 0xffffff )
			{
				r += n;
			}
			else
			{
				const TWord moduloTest = (m & 0x00ffff);

				// bit-reverse mode
				if( moduloTest == 0x0000 )
				{
					assert( 0 && "is used" );

					r = bitreverse24(r);
					n = bitreverse24(n);
					r += n;
					r &= 0x00ffffff;
					r = bitreverse24(r);
				}

				// modulo
				else if( moduloTest <= 0x007fff )
				{
					if( abs(signextend<int,24>(n)) > int(m+1) )
					{
						// the doc says it's only valid for N = P x (2 pow k), but assume the assembly is okay
//						LOG( "r " << std::hex << r << " + n " << std::hex << n << " = " << std::hex << ((r+n)&0x00ffffff) );
						r += n;
					}
					else
					{
						// find base address
						unsigned int zeroBits		= countZeroBitsReversed(m);

						const TWord moduloMask		= (1<<((sizeof(m)*CHAR_BIT)-zeroBits))-1;
						const TWord baseAddrMask	= ~moduloMask;

						const TWord baseAddr		= r&baseAddrMask;

						const TWord rNew 			= baseAddr | (((r+n)&moduloMask ) % (moduloTest+1));

//						LOG( "r " << std::hex << r << " + n " << std::hex << n << "(m=" << std::hex << m << ") = " << std::hex << rNew << " mask=" << std::hex << moduloMask << " baseAddrMask=" << std::hex << baseAddrMask << " baseAddr=" << std::hex << baseAddr );

						r = rNew;
					}
				}
				else
					LOG_ERR_NOTIMPLEMENTED( "AGU multiple Wrap-Around Modulo Modifier" );
			}
			r &= 0x00ffffff;
		}
	};
};
