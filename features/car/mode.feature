@routing @car @mode
Feature: Car - Mode flag
    Background:
        Given the profile "car"

    Scenario: Car - Mode when using a ferry
        Given the node map
            | a | b |   |
            |   | c | d |

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bc    |         | ferry | 0:01     |
            | cd    | primary |       |          |

        When I route I should get
            | from | to | route    | turns                       | modes |
            | a    | d  | ab,bc,cd | head,right,left,destination | 1,2,1 |
            | d    | a  | cd,bc,ab | head,right,left,destination | 1,2,1 |
            | c    | a  | bc,ab    | head,left,destination       | 2,1   |
            | d    | b  | cd,bc    | head,right,destination      | 1,2   |
            | a    | c  | ab,bc    | head,right,destination      | 1,2   |
            | b    | d  | bc,cd    | head,left,destination       | 2,1   |

    Scenario: Car - Snapping when using a ferry
        Given the node map
            | a | b |   | c | d |   | e | f |

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bcde  |         | ferry | 0:10     |
            | ef    | primary |       |          |

        When I route I should get
            | from | to | route | turns            | modes   | time  |
            | c    | d  | bcde  | head,destination | 2       | 600s  |


