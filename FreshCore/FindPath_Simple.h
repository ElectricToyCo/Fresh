//
//  FindPath_Simple.h
//  Fresh
//
//  Created by Jeff Wofford on 2024-10-04.
//  Copyright © 2024 The Electric Toy Co. All rights reserved.
//

#ifndef FindPath_Simple_h
#define FindPath_Simple_h

#include <vector>
#include <functional>

namespace fr
{
    std::vector< int > findPath( int start,
                                 int goal,
                                 std::function< std::vector< int >( int ) >&& nodeNeighbors,
                                 std::function< float( int ) >&& travelWeight,
                                 std::function< float( int ) >&& heuristic
                               )
    {

    }
}


#endif /* FindPath_Simple_h */
